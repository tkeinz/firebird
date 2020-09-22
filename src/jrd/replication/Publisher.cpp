/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2013 Dmitry Yemanov <dimitr@firebirdsql.org>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/jrd.h"
#include "../jrd/ods.h"
#include "../jrd/req.h"
#include "../jrd/tra.h"
#include "firebird/impl/blr.h"
#include "../jrd/trig.h"
#include "../jrd/Database.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../common/isc_proto.h"

#include "Publisher.h"
#include "Replicator.h"

using namespace Firebird;
using namespace Jrd;
using namespace Replication;

namespace
{
	// Generator RDB$BACKUP_HISTORY, although defined as system,
	// should be replicated similar to user-defined ones
	const int BACKUP_HISTORY_GENERATOR = 9;

	const char* NO_PLUGIN_ERROR = "Replication plugin %s is not found";
	const char* STOP_ERROR = "Replication is stopped due to critical error(s)";

	void logStatus(const Database* dbb, const ISC_STATUS* status, LogMsgType type)
	{
		string message;
		char temp[BUFFER_LARGE];

		while (fb_interpret(temp, sizeof(temp), &status))
		{
			if (!message.isEmpty())
				message += "\n\t";

			message += temp;
		}

		logOriginMessage(dbb->dbb_filename.c_str(), message, type);
	}

	void handleError(thread_db* tdbb, jrd_tra* transaction = nullptr, bool canThrow = true)
	{
		const auto dbb = tdbb->getDatabase();
		const auto attachment = tdbb->getAttachment();

		fb_assert(attachment->att_replicator.hasData());

		const auto config = dbb->replConfig();
		fb_assert(config);

		if (transaction && transaction->tra_replicator && config->disableOnError)
		{
			transaction->tra_replicator->dispose();
			transaction->tra_replicator = nullptr;
		}

		const auto status = attachment->att_replicator->getStatus();
		const auto state = status->getState();

		if (state & IStatus::STATE_WARNINGS)
		{
			if (config->logErrors)
				logStatus(dbb, status->getWarnings(), WARNING_MSG);
		}

		if (state & IStatus::STATE_ERRORS)
		{
			if (config->logErrors)
				logStatus(dbb, status->getErrors(), ERROR_MSG);

			if (config->disableOnError)
				logOriginMessage(dbb->dbb_filename, STOP_ERROR, ERROR_MSG);

			if (config->reportErrors && canThrow)
				status_exception::raise(status);
		}
	}

	IReplicatedSession* getReplicator(thread_db* tdbb, bool cleanupTransactions = false)
	{
		const auto attachment = tdbb->getAttachment();

		// Disable replication for system attachments

		if (attachment->isSystem())
			return nullptr;

		// Check whether replication is configured and enabled for this database

		const auto dbb = tdbb->getDatabase();
		if (!dbb->isReplicating(tdbb))
		{
			attachment->att_replicator = nullptr;
			return nullptr;
		}

		const auto config = dbb->replConfig();
		fb_assert(config);

		// Create a replicator object, unless it already exists

		if (attachment->att_replicator.hasData())
		{
			// If replication must be disabled after errors and the status is dirty,
			// then fake the replication being inactive

			const auto status = attachment->att_replicator->getStatus();
			const auto state = status->getState();

			if (config->disableOnError && (state & IStatus::STATE_ERRORS))
				return nullptr;

			status->init(); // reset the status
		}
		else
		{
			if (config->pluginName.empty())
			{
				auto& pool = *attachment->att_pool;
				const auto manager = dbb->replManager(true);
				const auto& guid = dbb->dbb_guid;
				const auto& userName = attachment->att_user->getUserName();

				attachment->att_replicator = FB_NEW Replicator(pool, manager, guid, userName);
			}
			else
			{
				GetPlugins<IReplicatedSession> plugins(IPluginManager::TYPE_REPLICATOR,
													   config->pluginName.c_str());
				if (!plugins.hasData())
				{
					string msg;
					msg.printf(NO_PLUGIN_ERROR, config->pluginName.c_str());
					logOriginMessage(dbb->dbb_filename, msg, ERROR_MSG);

					return nullptr;
				}

				attachment->att_replicator = plugins.plugin();
			}

			attachment->att_replicator->setAttachment(attachment->getInterface());
			if (cleanupTransactions)
				attachment->att_replicator->cleanupTransaction(0);
		}

		fb_assert(attachment->att_replicator.hasData());

		return attachment->att_replicator;
	}

	IReplicatedTransaction* getReplicator(thread_db* tdbb, jrd_tra* transaction)
	{
		// Disable replication for system and read-only transactions

		if (transaction->tra_flags & (TRA_system | TRA_readonly))
			return nullptr;

		// Check parent replicator presense
		// (this includes checking for the database-wise replication state)

		const auto replicator = getReplicator(tdbb);
		if (!replicator)
		{
			if (transaction->tra_replicator)
			{
				transaction->tra_replicator->dispose();
				transaction->tra_replicator = nullptr;
			}

			return nullptr;
		}

		// Create a replicator object, unless it already exists

		if (!transaction->tra_replicator && (transaction->tra_flags & TRA_replicating))
		{
			transaction->tra_replicator =
				replicator->startTransaction(transaction->getInterface(true),
											 transaction->tra_number);

			if (!transaction->tra_replicator)
				handleError(tdbb, transaction);
		}

		return transaction->tra_replicator;
	}

	bool matchTable(thread_db* tdbb, const MetaName& tableName)
	{
		const auto attachment = tdbb->getAttachment();
		const auto matcher = attachment->att_repl_matcher.get();

		return (!matcher || matcher->matchTable(tableName));
	}

	Record* upgradeRecord(thread_db* tdbb, jrd_rel* relation, Record* record)
	{
		const auto format = MET_current(tdbb, relation);

		if (record->getFormat()->fmt_version == format->fmt_version)
			return record;

		auto& pool = *tdbb->getDefaultPool();
		const auto newRecord = FB_NEW_POOL(pool) Record(pool, format);

		dsc orgDesc, newDesc;

		for (auto i = 0; i < newRecord->getFormat()->fmt_count; i++)
		{
			newRecord->clearNull(i);

			if (EVL_field(relation, newRecord, i, &newDesc))
			{
				if (EVL_field(relation, record, i, &orgDesc))
					MOV_move(tdbb, &orgDesc, &newDesc);
				else
					newRecord->setNull(i);
			}
		}

		return newRecord;
	}

	bool ensureSavepoints(thread_db* tdbb, jrd_tra* transaction)
	{
		const auto replicator = transaction->tra_replicator;

		// Replicate the entire stack of active savepoints (excluding priorly replicated),
		// starting with the oldest ones

		HalfStaticArray<Savepoint*, 16> stack;

		for (Savepoint::Iterator iter(transaction->tra_save_point); *iter; ++iter)
		{
			const auto savepoint = *iter;

			if (savepoint->isReplicated())
				break;

			stack.push(savepoint);
		}

		while (stack.hasData())
		{
			const auto savepoint = stack.pop();

			if (!replicator->startSavepoint())
			{
				handleError(tdbb, transaction);
				return false;
			}

			savepoint->markAsReplicated();
		}

		return true;
	}

	class ReplicatedRecordImpl :
		public AutoIface<IReplicatedRecordImpl<ReplicatedRecordImpl, CheckStatusWrapper> >,
		public IReplicatedFieldImpl<ReplicatedRecordImpl, CheckStatusWrapper>
	{
	public:
		ReplicatedRecordImpl(thread_db* tdbb, const jrd_rel* relation, const Record* record)
			: m_record(record),
			  m_relation(relation)
		{
		}

		~ReplicatedRecordImpl()
		{
		}

		unsigned getCount() override
		{
			const auto format = m_record->getFormat();
			return format->fmt_count;
		}

		const char* getName() override
		{
			const auto field = MET_get_field(m_relation, m_fieldIndex);
			return field ? field->fld_name.c_str() : nullptr;
		}

		unsigned getType() override
		{
			return m_fieldType;
		}

		unsigned getSubType() override
		{
			return m_desc->getSubType();
		}

		unsigned getScale() override
		{
			return m_desc->dsc_scale;
		}

		unsigned getLength() override
		{
			return m_fieldLength;
		}

		unsigned getCharSet() override
		{
			return m_desc->getCharSet();
		}

		const void* getData() override
		{
			if (m_record->isNull(m_fieldIndex))
				return nullptr;

			return m_record->getData() + (IPTR) m_desc->dsc_address;
		}

		IReplicatedField* getField(unsigned index) override
		{
			const auto format = m_record->getFormat();

			if (index >= format->fmt_count)
				return nullptr;

			const auto desc = &format->fmt_desc[index];

			if (desc->isUnknown() || !desc->dsc_address)
				return nullptr;

			m_desc = desc;
			m_fieldIndex = index;
			SLONG dummySubtype, dummyScale;
			desc->getSqlInfo(&m_fieldLength, &dummySubtype, &dummyScale, &m_fieldType);

			return this;
		}

		unsigned getRawLength() override
		{
			return m_record->getLength();
		}

		const unsigned char* getRawData() override
		{
			return m_record->getData();
		}

	private:
		const Record* const m_record;
		const jrd_rel* const m_relation;
		const dsc* m_desc = nullptr; // optimization
		unsigned m_fieldIndex = 0;
		SLONG m_fieldLength = 0;
		SLONG m_fieldType = 0;
	};
}


void REPL_attach(thread_db* tdbb, bool cleanupTransactions)
{
	const auto dbb = tdbb->getDatabase();
	const auto attachment = tdbb->getAttachment();

	const auto replConfig = dbb->replConfig();
	if (!replConfig)
		return;

	fb_assert(!attachment->att_repl_matcher);
	auto& pool = *attachment->att_pool;
	attachment->att_repl_matcher = FB_NEW_POOL(pool)
		TableMatcher(pool, replConfig->includeFilter, replConfig->excludeFilter);

	fb_assert(!attachment->att_replicator);
	if (cleanupTransactions)
		getReplicator(tdbb, cleanupTransactions);
	// else defer creation of replicator till really needed
}

void REPL_trans_prepare(thread_db* tdbb, jrd_tra* transaction)
{
	// There is no need to call getReplicator() and make it to create a new ReplicatedTransaction
	// just to end it up at once.
	const auto replicator = transaction->tra_replicator;
	if (!replicator)
		return;

	if (!replicator->prepare())
		handleError(tdbb, transaction);
}

void REPL_trans_commit(thread_db* tdbb, jrd_tra* transaction)
{
	const auto replicator = transaction->tra_replicator;
	if (!replicator)
		return;

	if (!replicator->commit())
	{
		// Commit is a terminal routine, we cannot throw here
		handleError(tdbb, transaction, false);
	}

	if (transaction->tra_replicator)
	{
		transaction->tra_replicator->dispose();
		transaction->tra_replicator = nullptr;
	}
}

void REPL_trans_rollback(thread_db* tdbb, jrd_tra* transaction)
{
	const auto replicator = transaction->tra_replicator;
	if (!replicator)
		return;

	if (!replicator->rollback())
	{
		// Rollback is a terminal routine, we cannot throw here
		handleError(tdbb, transaction, false);
	}

	if (transaction->tra_replicator)
	{
		transaction->tra_replicator->dispose();
		transaction->tra_replicator = nullptr;
	}
}

void REPL_trans_cleanup(Jrd::thread_db* tdbb, TraNumber number)
{
	const auto replicator = getReplicator(tdbb);
	if (!replicator)
		return;

	if (!replicator->cleanupTransaction(number))
		handleError(tdbb);
}

void REPL_save_cleanup(thread_db* tdbb, jrd_tra* transaction,
				  	   const Savepoint* savepoint, bool undo)
{
	if (tdbb->tdbb_flags & (TDBB_dont_post_dfw | TDBB_repl_in_progress))
		return;

	if (!transaction->tra_save_point->isReplicated())
		return;

	const auto replicator = transaction->tra_replicator;
	if (!replicator)
		return;

	if (undo)
	{
		if (!replicator->rollbackSavepoint())
			handleError(tdbb, transaction);
	}
	else
	{
		if (!replicator->releaseSavepoint())
			handleError(tdbb, transaction);
	}
}

void REPL_store(thread_db* tdbb, const record_param* rpb, jrd_tra* transaction)
{
	if (tdbb->tdbb_flags & (TDBB_dont_post_dfw | TDBB_repl_in_progress))
		return;

	const auto relation = rpb->rpb_relation;
	fb_assert(relation);

	if (relation->isTemporary())
		return;

	if (!relation->isSystem())
	{
		if (!relation->isReplicating(tdbb))
			return;

		if (!matchTable(tdbb, relation->rel_name))
			return;
	}

	const auto replicator = getReplicator(tdbb, transaction);
	if (!replicator)
		return;

	const auto record = upgradeRecord(tdbb, relation, rpb->rpb_record);
	fb_assert(record);

	// This temporary auto-pointer is just to delete a temporary record
	AutoPtr<Record> cleanupRecord(record != rpb->rpb_record ? record : nullptr);
	AutoSetRestoreFlag<ULONG> noRecursion(&tdbb->tdbb_flags, TDBB_repl_in_progress, true);

	if (!ensureSavepoints(tdbb, transaction))
		return;

	ReplicatedRecordImpl replRecord(tdbb, relation, record);

	if (!replicator->insertRecord(relation->rel_name.c_str(), &replRecord))
		handleError(tdbb, transaction);
}

void REPL_modify(thread_db* tdbb, const record_param* orgRpb,
				 const record_param* newRpb, jrd_tra* transaction)
{
	if (tdbb->tdbb_flags & (TDBB_dont_post_dfw | TDBB_repl_in_progress))
		return;

	const auto relation = newRpb->rpb_relation;
	fb_assert(relation);

	if (relation->isTemporary())
		return;

	if (!relation->isSystem())
	{
		if (!relation->isReplicating(tdbb))
			return;

		if (!matchTable(tdbb, relation->rel_name))
			return;
	}

	const auto replicator = getReplicator(tdbb, transaction);
	if (!replicator)
		return;

	const auto newRecord = upgradeRecord(tdbb, relation, newRpb->rpb_record);
	fb_assert(newRecord);

	const auto orgRecord = upgradeRecord(tdbb, relation, orgRpb->rpb_record);
	fb_assert(orgRecord);

	// These temporary auto-pointers are just to delete temporary records
	AutoPtr<Record> cleanupOrgRecord(orgRecord != orgRpb->rpb_record ? orgRecord : nullptr);
	AutoPtr<Record> cleanupNewRecord(newRecord != newRpb->rpb_record ? newRecord : nullptr);

	const auto orgLength = orgRecord->getLength();
	const auto newLength = newRecord->getLength();

	// Ignore dummy updates
	if (orgLength == newLength &&
		!memcmp(orgRecord->getData(), newRecord->getData(), orgLength))
	{
		return;
	}

	AutoSetRestoreFlag<ULONG> noRecursion(&tdbb->tdbb_flags, TDBB_repl_in_progress, true);

	if (!ensureSavepoints(tdbb, transaction))
		return;

	ReplicatedRecordImpl replOrgRecord(tdbb, relation, orgRecord);
	ReplicatedRecordImpl replNewRecord(tdbb, relation, newRecord);

	if (!replicator->updateRecord(relation->rel_name.c_str(), &replOrgRecord, &replNewRecord))
		handleError(tdbb, transaction);
}


void REPL_erase(thread_db* tdbb, const record_param* rpb, jrd_tra* transaction)
{
	if (tdbb->tdbb_flags & (TDBB_dont_post_dfw | TDBB_repl_in_progress))
		return;

	const auto relation = rpb->rpb_relation;
	fb_assert(relation);

	if (relation->isTemporary())
		return;

	if (!relation->isSystem())
	{
		if (!relation->isReplicating(tdbb))
			return;

		if (!matchTable(tdbb, relation->rel_name))
			return;
	}

	const auto replicator = getReplicator(tdbb, transaction);
	if (!replicator)
		return;

	const auto record = upgradeRecord(tdbb, relation, rpb->rpb_record);
	fb_assert(record);

	// This temporary auto-pointer is just to delete a temporary record
	AutoPtr<Record> cleanupRecord(record != rpb->rpb_record ? record : nullptr);
	AutoSetRestoreFlag<ULONG> noRecursion(&tdbb->tdbb_flags, TDBB_repl_in_progress, true);

	if (!ensureSavepoints(tdbb, transaction))
		return;

	ReplicatedRecordImpl replRecord(tdbb, relation, record);

	if (!replicator->deleteRecord(relation->rel_name.c_str(), &replRecord))
		handleError(tdbb, transaction);
}

void REPL_gen_id(thread_db* tdbb, SLONG genId, SINT64 value)
{
	if (tdbb->tdbb_flags & (TDBB_dont_post_dfw | TDBB_repl_in_progress))
		return;

	if (genId == 0) // special case: ignore RDB$GENERATORS
		return;

	// Ignore other system generators, except RDB$BACKUP_HISTORY
	if (genId != BACKUP_HISTORY_GENERATOR)
	{
		for (auto generator = generators; generator->gen_name; generator++)
		{
			if (generator->gen_id == genId)
				return;
		}
	}

	const auto replicator = getReplicator(tdbb);
	if (!replicator)
		return;

	const auto attachment = tdbb->getAttachment();

	MetaName genName;
	if (!attachment->att_generators.lookup(genId, genName))
	{
		MET_lookup_generator_id(tdbb, genId, genName, nullptr);
		attachment->att_generators.store(genId, genName);
	}

	AutoSetRestoreFlag<ULONG> noRecursion(&tdbb->tdbb_flags, TDBB_repl_in_progress, true);

	if (!replicator->setSequence(genName.c_str(), value))
		handleError(tdbb);
}

void REPL_exec_sql(thread_db* tdbb, jrd_tra* transaction, const string& sql)
{
	fb_assert(tdbb->tdbb_flags & TDBB_repl_in_progress);

	if (tdbb->tdbb_flags & TDBB_dont_post_dfw)
		return;

	const auto replicator = getReplicator(tdbb, transaction);
	if (!replicator)
		return;

	if (!ensureSavepoints(tdbb, transaction))
		return;

	const auto attachment = tdbb->getAttachment();
	const auto charset = attachment->att_charset;

	// This place is already protected from recursion in calling code

	if (!replicator->executeSqlIntl(charset, sql.c_str()))
		handleError(tdbb, transaction);
}

void REPL_log_switch(thread_db* tdbb)
{
	const auto dbb = tdbb->getDatabase();

	const auto replMgr = dbb->replManager();
	if (!replMgr)
		return;

	replMgr->forceLogSwitch();
}
