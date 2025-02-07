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
 *  The Original Code was created by Mark O'Donohue
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2002 Mark O'Donohue <skywalker@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *  2005.05.19 Claudio Valderrama: signal tokens that aren't reserved in the
 *      engine thanks to special handling.
 *  Adriano dos Santos Fernandes
 */

#include "firebird.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#define _yacc_defines_yystype
#include "gen/parse.h"
#include "keywords.h"

// CVC: The latest column indicates whether the token has special handling in
// the parser. If it does, KEYWORD_stringIsAToken will return false.
// I discovered isql was being fooled and put double quotes around those
// special cases unnecessarily.

static const TOK tokens[] =
{
	{TOK_NOT_LSS, "!<", false},
	{TOK_NEQ, "!=", false},
	{TOK_NOT_GTR, "!>", false},
	{'(', "(", false},
	{')', ")", false},
	{',', ",", false},
	{'<', "<", false},
	{TOK_LEQ, "<=", false},
	{TOK_NEQ, "<>", false},	// Alias of !=
	{'=', "=", false},
	{'>', ">", false},
	{TOK_GEQ, ">=", false},
	{TOK_BIND_PARAM, ":=", false},
	{TOK_ABS, "ABS", true},
	{TOK_ABSOLUTE, "ABSOLUTE", true},
	{TOK_ACCENT, "ACCENT", true},
	{TOK_ACOS, "ACOS", true},
	{TOK_ACOSH, "ACOSH", true},
	{TOK_ACTION, "ACTION", true},
	{TOK_ACTIVE, "ACTIVE", true},
	{TOK_ADD, "ADD", false},
	{TOK_ADMIN, "ADMIN", false},
	{TOK_AFTER, "AFTER", true},
	{TOK_ALL, "ALL", false},
	{TOK_ALTER, "ALTER", false},
	{TOK_ALWAYS, "ALWAYS", true},
	{TOK_AND, "AND", false},
	{TOK_ANY, "ANY", false},
	{TOK_AS, "AS", false},
	{TOK_ASC, "ASC", true},	// Alias of ASCENDING
	{TOK_ASC, "ASCENDING", true},
	{TOK_ASCII_CHAR, "ASCII_CHAR", true},
	{TOK_ASCII_VAL, "ASCII_VAL", true},
	{TOK_ASIN, "ASIN", true},
	{TOK_ASINH, "ASINH", true},
	{TOK_AT, "AT", false},
	{TOK_ATAN, "ATAN", true},
	{TOK_ATAN2, "ATAN2", true},
	{TOK_ATANH, "ATANH", true},
	{TOK_AUTO, "AUTO", true},
	{TOK_AUTONOMOUS, "AUTONOMOUS", true},
	{TOK_AVG, "AVG", false},
	{TOK_BACKUP, "BACKUP", true},
	{TOK_BASE64_DECODE, "BASE64_DECODE", true},
	{TOK_BASE64_ENCODE, "BASE64_ENCODE", true},
	{TOK_BEFORE, "BEFORE", true},
	{TOK_BEGIN, "BEGIN", false},
	{TOK_BETWEEN, "BETWEEN", false},
	{TOK_BIGINT, "BIGINT", false},
	{TOK_BIN_AND, "BIN_AND", true},
	{TOK_BIN_NOT, "BIN_NOT", true},
	{TOK_BIN_OR, "BIN_OR", true},
	{TOK_BIN_SHL, "BIN_SHL", true},
	{TOK_BIN_SHR, "BIN_SHR", true},
	{TOK_BIN_XOR, "BIN_XOR", true},
	{TOK_BINARY, "BINARY", false},
	{TOK_BIND, "BIND", true},
	{TOK_BIT_LENGTH, "BIT_LENGTH", false},
	{TOK_BLOB, "BLOB", false},
	{TOK_BLOCK, "BLOCK", true},
	{TOK_BODY, "BODY", true},
	{TOK_BOOLEAN, "BOOLEAN", false},
	{TOK_BOTH, "BOTH", false},
	{TOK_BREAK, "BREAK", true},
	{TOK_BY, "BY", false},
	{TOK_CALLER, "CALLER", true},
	{TOK_CASCADE, "CASCADE", true},
	{TOK_CASE, "CASE", false},
	{TOK_CAST, "CAST", false},
	{TOK_CEIL, "CEIL", true},	// Alias of CEILING
	{TOK_CEIL, "CEILING", true},
	{TOK_CHAR, "CHAR", false},
	{TOK_CHAR_LENGTH, "CHAR_LENGTH", false},
	{TOK_CHAR_TO_UUID, "CHAR_TO_UUID", true},
	{TOK_CHARACTER, "CHARACTER", false},
	{TOK_CHARACTER_LENGTH, "CHARACTER_LENGTH", false},
	{TOK_CHECK, "CHECK", false},
	{TOK_CLEAR, "CLEAR", true},
	{TOK_CLOSE, "CLOSE", false},
	{TOK_COALESCE, "COALESCE", true},
	{TOK_COLLATE, "COLLATE", false},
	{TOK_COLLATION, "COLLATION", true},
	{TOK_COLUMN, "COLUMN", false},
	{TOK_COMMENT, "COMMENT", false},
	{TOK_COMMIT, "COMMIT", false},
	{TOK_COMMITTED, "COMMITTED", true},
	{TOK_COMMON, "COMMON", true},
	{TOK_COMPARE_DECFLOAT, "COMPARE_DECFLOAT", true},
	{TOK_COMPUTED, "COMPUTED", true},
	{TOK_CONDITIONAL, "CONDITIONAL", true},
	{TOK_CONNECT, "CONNECT", false},
	{TOK_CONNECTIONS, "CONNECTIONS", true},
	{TOK_CONSISTENCY, "CONSISTENCY", true},
	{TOK_CONSTRAINT, "CONSTRAINT", false},
	{TOK_CONTAINING, "CONTAINING", true},
	{TOK_CONTINUE, "CONTINUE", true},
	{TOK_CORR, "CORR", false},
	{TOK_COS, "COS", true},
	{TOK_COSH, "COSH", true},
	{TOK_COT, "COT", true},
	{TOK_COUNT, "COUNT", false},
	{TOK_COUNTER, "COUNTER", true},
	{TOK_COVAR_POP, "COVAR_POP", false},
	{TOK_COVAR_SAMP, "COVAR_SAMP", false},
	{TOK_CREATE, "CREATE", false},
	{TOK_CROSS, "CROSS", false},
	{TOK_CRYPT_HASH, "CRYPT_HASH", true},
	{TOK_CSTRING, "CSTRING", true},
	{TOK_CTR_BIG_ENDIAN, "CTR_BIG_ENDIAN", true},
	{TOK_CTR_LENGTH, "CTR_LENGTH", true},
	{TOK_CTR_LITTLE_ENDIAN, "CTR_LITTLE_ENDIAN", true},
	{TOK_CUME_DIST, "CUME_DIST", true},
	{TOK_CURRENT, "CURRENT", false},
	{TOK_CURRENT_CONNECTION, "CURRENT_CONNECTION", false},
	{TOK_CURRENT_DATE, "CURRENT_DATE", false},
	{TOK_CURRENT_ROLE, "CURRENT_ROLE", false},
	{TOK_CURRENT_TIME, "CURRENT_TIME", false},
	{TOK_CURRENT_TIMESTAMP, "CURRENT_TIMESTAMP", false},
	{TOK_CURRENT_TRANSACTION, "CURRENT_TRANSACTION", false},
	{TOK_CURRENT_USER, "CURRENT_USER", false},
	{TOK_CURSOR, "CURSOR", false},
	{TOK_DATABASE, "DATABASE", true},
	{TOK_DATA, "DATA", true},
	{TOK_DATE, "DATE", false},
	{TOK_DATEADD, "DATEADD", true},
	{TOK_DATEDIFF, "DATEDIFF", true},
	{TOK_DAY, "DAY", false},
	{TOK_DDL, "DDL", true},
	{TOK_DEBUG, "DEBUG", true},
	{TOK_DEC, "DEC", false},
	{TOK_DECFLOAT, "DECFLOAT", false},
	{TOK_DECIMAL, "DECIMAL", false},
	{TOK_DECLARE, "DECLARE", false},
	{TOK_DECODE, "DECODE", true},
	{TOK_DECRYPT, "DECRYPT", true},
	{TOK_DEFAULT, "DEFAULT", false},
	{TOK_DEFINER, "DEFINER", true},
	{TOK_DELETE, "DELETE", false},
	{TOK_DELETING, "DELETING", false},
	{TOK_DENSE_RANK, "DENSE_RANK", true},
	{TOK_DESC, "DESC", true},	// Alias of DESCENDING
	{TOK_DESC, "DESCENDING", true},
	{TOK_DESCRIPTOR,	"DESCRIPTOR", true},
	{TOK_DETERMINISTIC, "DETERMINISTIC", false},
	{TOK_DIFFERENCE, "DIFFERENCE", true},
	{TOK_DISABLE, "DISABLE", true},
	{TOK_DISCONNECT, "DISCONNECT", false},
	{TOK_DISTINCT, "DISTINCT", false},
	{TOK_DO, "DO", true},
	{TOK_DOMAIN, "DOMAIN", true},
	{TOK_DOUBLE, "DOUBLE", false},
	{TOK_DROP, "DROP", false},
	{TOK_ELSE, "ELSE", false},
	{TOK_ENABLE, "ENABLE", true},
	{TOK_ENCRYPT, "ENCRYPT", true},
	{TOK_END, "END", false},
	{TOK_ENGINE, "ENGINE", true},
	{TOK_ENTRY_POINT, "ENTRY_POINT", true},
	{TOK_ESCAPE, "ESCAPE", false},
	{TOK_EXCEPTION, "EXCEPTION", true},
	{TOK_EXCESS, "EXCESS", true},
	{TOK_EXCLUDE, "EXCLUDE", true},
	{TOK_EXECUTE, "EXECUTE", false},
	{TOK_EXISTS, "EXISTS", false},
	{TOK_EXIT, "EXIT", true},
	{TOK_EXP, "EXP", true},
	{TOK_EXTENDED, "EXTENDED", true},
	{TOK_EXTERNAL, "EXTERNAL", false},
	{TOK_EXTRACT, "EXTRACT", false},
	{TOK_FALSE, "FALSE", false},
	{TOK_FETCH, "FETCH", false},
	{TOK_FILE, "FILE", true},
	{TOK_FILTER, "FILTER", false},
	{TOK_FIRST, "FIRST", true},
	{TOK_FIRST_DAY, "FIRST_DAY", true},
	{TOK_FIRST_VALUE, "FIRST_VALUE", true},
	{TOK_FIRSTNAME, "FIRSTNAME", true},
	{TOK_FLOAT, "FLOAT", false},
	{TOK_FLOOR, "FLOOR", true},
	{TOK_FOLLOWING, "FOLLOWING", true},
	{TOK_FOR, "FOR", false},
	{TOK_FOREIGN, "FOREIGN", false},
	{TOK_FREE_IT, "FREE_IT", true},
	{TOK_FROM, "FROM", false},
	{TOK_FULL, "FULL", false},
	{TOK_FUNCTION, "FUNCTION", false},
	{TOK_GDSCODE, "GDSCODE", false},
	{TOK_GENERATED, "GENERATED", true},
	{TOK_GENERATOR, "GENERATOR", true},
	{TOK_GEN_ID, "GEN_ID", true},
	{TOK_GEN_UUID, "GEN_UUID", true},
	{TOK_GLOBAL, "GLOBAL", false},
	{TOK_GRANT, "GRANT", false},
	{TOK_GRANTED, "GRANTED", true},
	{TOK_GROUP, "GROUP", false},
	{TOK_HASH, "HASH", true},
	{TOK_HAVING, "HAVING", false},
	{TOK_HEX_DECODE, "HEX_DECODE", true},
	{TOK_HEX_ENCODE, "HEX_ENCODE", true},
	{TOK_HOUR, "HOUR", false},
	{TOK_IDENTITY, "IDENTITY", true},
	{TOK_IDLE, "IDLE", true},
	{TOK_IF, "IF", true},
	{TOK_IGNORE, "IGNORE", true},
	{TOK_IIF, "IIF", true},
	{TOK_IN, "IN", false},
	{TOK_INACTIVE, "INACTIVE", true},
	{TOK_INCLUDE, "INCLUDE", true},
	{TOK_INCREMENT, "INCREMENT", true},
	{TOK_INDEX, "INDEX", false},
	{TOK_INNER, "INNER", false},
	{TOK_INPUT_TYPE, "INPUT_TYPE", true},
	{TOK_INSENSITIVE, "INSENSITIVE", false},
	{TOK_INSERT, "INSERT", false},
	{TOK_INSERTING, "INSERTING", false},
	{TOK_INT, "INT", false},
	{TOK_INT128, "INT128", false},
	{TOK_INTEGER, "INTEGER", false},
	{TOK_INTO, "INTO", false},
	{TOK_INVOKER, "INVOKER", true},
	{TOK_IS, "IS", false},
	{TOK_ISOLATION, "ISOLATION", true},
	{TOK_IV, "IV", true},
	{TOK_JOIN, "JOIN", false},
	{TOK_KEY, "KEY", true},
	{TOK_LAG, "LAG", true},
	{TOK_LAST, "LAST", true},
	{TOK_LAST_DAY, "LAST_DAY", true},
	{TOK_LAST_VALUE, "LAST_VALUE", true},
	{TOK_LASTNAME, "LASTNAME", true},
	{TOK_LEAD, "LEAD", true},
	{TOK_LEADING, "LEADING", false},
	{TOK_LEAVE, "LEAVE", true},
	{TOK_LEFT, "LEFT", false},
	{TOK_LEGACY, "LEGACY", true},
	{TOK_LENGTH, "LENGTH", true},
	{TOK_LEVEL, "LEVEL", true},
	{TOK_LIFETIME, "LIFETIME", true},
	{TOK_LIKE, "LIKE", false},
	{TOK_LIMBO, "LIMBO", true},
	{TOK_LINGER, "LINGER", true},
	{TOK_LIST, "LIST", true},
	{TOK_LN, "LN", true},
	{TOK_LATERAL, "LATERAL", false},
	{TOK_LOCAL, "LOCAL", false},
	{TOK_LOCALTIME, "LOCALTIME", false},
	{TOK_LOCALTIMESTAMP, "LOCALTIMESTAMP", false},
	{TOK_LOCK, "LOCK", true},
	{TOK_LOG, "LOG", true},
	{TOK_LOG10, "LOG10", true},
	{TOK_LONG, "LONG", false},
	{TOK_LOWER, "LOWER", false},
	{TOK_LPAD, "LPAD", true},
	{TOK_LPARAM, "LPARAM", true},
	{TOK_MAKE_DBKEY, "MAKE_DBKEY", true},
	{TOK_MANUAL, "MANUAL", true},
	{TOK_MAPPING, "MAPPING", true},
	{TOK_MATCHED, "MATCHED", true},
	{TOK_MATCHING, "MATCHING", true},
	{TOK_MAXIMUM, "MAX", false},
	{TOK_MAXVALUE, "MAXVALUE", true},
	{TOK_MERGE, "MERGE", false},
	{TOK_MESSAGE, "MESSAGE", true},
	{TOK_MILLISECOND, "MILLISECOND", true},
	{TOK_MIDDLENAME, "MIDDLENAME", true},
	{TOK_MINIMUM, "MIN", false},
	{TOK_MINUTE, "MINUTE", false},
	{TOK_MINVALUE, "MINVALUE", true},
	{TOK_MOD, "MOD", true},
	{TOK_MODE, "MODE", true},
	{TOK_MODULE_NAME, "MODULE_NAME", true},
	{TOK_MONTH, "MONTH", false},
	{TOK_NAME, "NAME", true},
	{TOK_NAMES, "NAMES", true},
	{TOK_NATIONAL, "NATIONAL", false},
	{TOK_NATIVE, "NATIVE", true},
	{TOK_NATURAL, "NATURAL", false},
	{TOK_NCHAR, "NCHAR", false},
	{TOK_NEXT, "NEXT", true},
	{TOK_NO, "NO", false},
	{TOK_NORMALIZE_DECFLOAT, "NORMALIZE_DECFLOAT", true},
	{TOK_NOT, "NOT", false},
	{TOK_NTH_VALUE, "NTH_VALUE", true},
	{TOK_NTILE, "NTILE", true},
	{TOK_NULLIF, "NULLIF", true},
	{TOK_NULL, "NULL", false},
	{TOK_NULLS, "NULLS", true},
	{TOK_NUMBER, "NUMBER", true},
	{TOK_NUMERIC, "NUMERIC", false},
	{TOK_OCTET_LENGTH, "OCTET_LENGTH", false},
	{TOK_OF, "OF", false},
	{TOK_OFFSET, "OFFSET", false},
	{TOK_OLDEST, "OLDEST", true},
	{TOK_ON, "ON", false},
	{TOK_ONLY, "ONLY", false},
	{TOK_OPEN, "OPEN", false},
	{TOK_OPTION, "OPTION", true},
	{TOK_OR, "OR", false},
	{TOK_ORDER, "ORDER", false},
	{TOK_OS_NAME, "OS_NAME", true},
	{TOK_OTHERS, "OTHERS", true},
	{TOK_OUTER, "OUTER", false},
	{TOK_OUTPUT_TYPE, "OUTPUT_TYPE", true},
	{TOK_OVER, "OVER", false},
	{TOK_OVERFLOW, "OVERFLOW", true},
	{TOK_OVERLAY, "OVERLAY", true},
	{TOK_OVERRIDING, "OVERRIDING", true},
	{TOK_PACKAGE, "PACKAGE", true},
	{TOK_PAD, "PAD", true},
	{TOK_PAGE, "PAGE", true},
	{TOK_PAGES, "PAGES", true},
	{TOK_PAGE_SIZE, "PAGE_SIZE", true},
	{TOK_PARAMETER, "PARAMETER", false},
	{TOK_PARTITION, "PARTITION", true},
	{TOK_PASSWORD, "PASSWORD", true},
	{TOK_PERCENT_RANK, "PERCENT_RANK", true},
	{TOK_PI, "PI", true},
	{TOK_PLACING, "PLACING", true},
	{TOK_PLAN, "PLAN", false},
	{TOK_PLUGIN, "PLUGIN", true},
	{TOK_POOL, "POOL", true},
	{TOK_POSITION, "POSITION", false},
	{TOK_POST_EVENT, "POST_EVENT", false},
	{TOK_POWER, "POWER", true},
	{TOK_PRECEDING, "PRECEDING", true},
	{TOK_PRECISION, "PRECISION", false},
	{TOK_PRESERVE, "PRESERVE", true},
	{TOK_PRIMARY, "PRIMARY", false},
	{TOK_PRIOR, "PRIOR", true},
	{TOK_PRIVILEGE, "PRIVILEGE", true},
	{TOK_PRIVILEGES, "PRIVILEGES", true},
	{TOK_PROCEDURE, "PROCEDURE", false},
	{TOK_PROTECTED, "PROTECTED", true},
	{TOK_PUBLICATION, "PUBLICATION", false},
	{TOK_QUANTIZE, "QUANTIZE", true},
	{TOK_RAND, "RAND", true},
	{TOK_RANGE, "RANGE", true},
	{TOK_RANK, "RANK", true},
	{TOK_DB_KEY, "RDB$DB_KEY", false},
	{TOK_RDB_ERROR, "RDB$ERROR", false},
	{TOK_RDB_GET_CONTEXT, "RDB$GET_CONTEXT", false},
	{TOK_RDB_GET_TRANSACTION_CN, "RDB$GET_TRANSACTION_CN", false},
	{TOK_RDB_RECORD_VERSION, "RDB$RECORD_VERSION", false},
	{TOK_RDB_ROLE_IN_USE, "RDB$ROLE_IN_USE", false},
	{TOK_RDB_SET_CONTEXT, "RDB$SET_CONTEXT", false},
	{TOK_RDB_SYSTEM_PRIVILEGE, "RDB$SYSTEM_PRIVILEGE", false},
	{TOK_READ, "READ", true},
	{TOK_REAL, "REAL", false},
	{TOK_VERSION, "RECORD_VERSION", false},
	{TOK_RECREATE, "RECREATE", false},
	{TOK_RECURSIVE, "RECURSIVE", false},
	{TOK_REFERENCES, "REFERENCES", false},
	{TOK_REGR_AVGX, "REGR_AVGX", false},
	{TOK_REGR_AVGY, "REGR_AVGY", false},
	{TOK_REGR_COUNT, "REGR_COUNT", false},
	{TOK_REGR_INTERCEPT, "REGR_INTERCEPT", false},
	{TOK_REGR_R2, "REGR_R2", false},
	{TOK_REGR_SLOPE, "REGR_SLOPE", false},
	{TOK_REGR_SXX, "REGR_SXX", false},
	{TOK_REGR_SXY, "REGR_SXY", false},
	{TOK_REGR_SYY, "REGR_SYY", false},
	{TOK_RELATIVE, "RELATIVE", true},
	{TOK_RELEASE, "RELEASE", false},
	{TOK_REPLACE, "REPLACE", true},
	{TOK_REQUESTS, "REQUESTS", true},
	{TOK_RESERVING, "RESERV", true},	// Alias of RESERVING
	{TOK_RESERVING, "RESERVING", true},
	{TOK_RESET, "RESET", true},
	{TOK_RESETTING, "RESETTING", false},
	{TOK_RESTART, "RESTART", true},
	{TOK_RESTRICT, "RESTRICT", true},
	{TOK_RETAIN, "RETAIN", true},
	{TOK_RETURN, "RETURN", false},
	{TOK_RETURNING, "RETURNING", true},
	{TOK_RETURNING_VALUES, "RETURNING_VALUES", false},
	{TOK_RETURNS, "RETURNS", false},
	{TOK_REVERSE, "REVERSE", true},
	{TOK_REVOKE, "REVOKE", false},
	{TOK_RIGHT, "RIGHT", false},
	{TOK_ROLE, "ROLE", true},
	{TOK_ROLLBACK, "ROLLBACK", false},
	{TOK_ROUND, "ROUND", true},
	{TOK_ROW, "ROW", false},
	{TOK_ROW_COUNT, "ROW_COUNT", false},
	{TOK_ROW_NUMBER, "ROW_NUMBER", true},
	{TOK_ROWS, "ROWS", false},
	{TOK_RPAD, "RPAD", true},
	{TOK_RSA_DECRYPT, "RSA_DECRYPT", true},
	{TOK_RSA_ENCRYPT, "RSA_ENCRYPT", true},
	{TOK_RSA_PRIVATE, "RSA_PRIVATE", true},
	{TOK_RSA_PUBLIC, "RSA_PUBLIC", true},
	{TOK_RSA_SIGN_HASH, "RSA_SIGN_HASH", true},
	{TOK_RSA_VERIFY_HASH, "RSA_VERIFY_HASH", true},
	{TOK_SALT_LENGTH, "SALT_LENGTH", true},
	{TOK_SAVEPOINT, "SAVEPOINT", false},
	{TOK_SCALAR_ARRAY, "SCALAR_ARRAY", true},
	{TOK_DATABASE, "SCHEMA", false},	// Alias of DATABASE
	{TOK_SCROLL, "SCROLL", false},
	{TOK_SECOND, "SECOND", false},
	{TOK_SECURITY, "SECURITY", true},
	{TOK_SEGMENT, "SEGMENT", true},
	{TOK_SELECT, "SELECT", false},
	{TOK_SENSITIVE, "SENSITIVE", false},
	{TOK_SEQUENCE, "SEQUENCE", true},
	{TOK_SERVERWIDE, "SERVERWIDE", true},
	{TOK_SESSION, "SESSION", true},
	{TOK_SET, "SET", false},
	{TOK_SHADOW, "SHADOW", true},
	{TOK_SHARED, "SHARED", true},
	{TOK_SIGN, "SIGN", true},
	{TOK_SIGNATURE, "SIGNATURE", true},
	{TOK_SIMILAR, "SIMILAR", false},
	{TOK_SIN, "SIN", true},
	{TOK_SINGULAR, "SINGULAR", true},
	{TOK_SINH, "SINH", true},
	{TOK_SIZE, "SIZE", true},
	{TOK_SKIP, "SKIP", true},
	{TOK_SMALLINT, "SMALLINT", false},
	{TOK_SNAPSHOT, "SNAPSHOT", true},
	{TOK_SOME, "SOME", false},
	{TOK_SORT, "SORT", true},
	{TOK_SOURCE, "SOURCE", true},
	{TOK_SPACE, "SPACE", true},
	{TOK_SQL, "SQL", true},
	{TOK_SQLCODE, "SQLCODE", false},
	{TOK_SQLSTATE, "SQLSTATE", false},
	{TOK_SQRT, "SQRT", true},
	{TOK_STABILITY, "STABILITY", true},
	{TOK_START, "START", false},
	{TOK_STARTING, "STARTING", true},
	{TOK_STARTING, "STARTS", true},	// Alias of STARTING
	{TOK_STATEMENT, "STATEMENT", true},
	{TOK_STATISTICS, "STATISTICS", true},
	{TOK_STDDEV_POP, "STDDEV_POP", false},
	{TOK_STDDEV_SAMP, "STDDEV_SAMP", false},
	{TOK_SUBSTRING,	"SUBSTRING", true},
	{TOK_SUB_TYPE, "SUB_TYPE", true},
	{TOK_SUM, "SUM", false},
	{TOK_SUSPEND, "SUSPEND", true},
	{TOK_SYSTEM, "SYSTEM", true},
	{TOK_TABLE, "TABLE", false},
	{TOK_TAGS, "TAGS", true},
	{TOK_TAN, "TAN", true},
	{TOK_TANH, "TANH", true},
	{TOK_TEMPORARY, "TEMPORARY", true},
	{TOK_THEN, "THEN", false},
	{TOK_TIES, "TIES", true},
	{TOK_TIME, "TIME", false},
	{TOK_TIMESTAMP, "TIMESTAMP", false},
	{TOK_TIMEOUT, "TIMEOUT", true},
	{TOK_TIMEZONE_HOUR, "TIMEZONE_HOUR", false},
	{TOK_TIMEZONE_MINUTE, "TIMEZONE_MINUTE", false},
	{TOK_TIMEZONE_NAME, "TIMEZONE_NAME", true},
	{TOK_TO, "TO", false},
	{TOK_TOTALORDER, "TOTALORDER", true},
	{TOK_TRAILING, "TRAILING", false},
	{TOK_TRANSACTION, "TRANSACTION", true},
	{TOK_TRAPS, "TRAPS", true},
	{TOK_TRIGGER, "TRIGGER", false},
	{TOK_TRIM, "TRIM", false},
	{TOK_TRUE, "TRUE", false},
	{TOK_TRUNC, "TRUNC", true},
	{TOK_TRUSTED, "TRUSTED", true},
	{TOK_TWO_PHASE, "TWO_PHASE", true},
	{TOK_TYPE, "TYPE", true},
	{TOK_UNBOUNDED, "UNBOUNDED", false},
	{TOK_UNCOMMITTED, "UNCOMMITTED", true},
	{TOK_UNDO, "UNDO", true},
	{TOK_UNICODE_CHAR, "UNICODE_CHAR", true},
	{TOK_UNICODE_VAL, "UNICODE_VAL", true},
	{TOK_UNION, "UNION", false},
	{TOK_UNIQUE, "UNIQUE", false},
	{TOK_UNKNOWN, "UNKNOWN", false},
	{TOK_UPDATE, "UPDATE", false},
	{TOK_UPDATING, "UPDATING", false},
	{TOK_UPPER, "UPPER", false},
	{TOK_USAGE, "USAGE", true},
	{TOK_USER, "USER", false},
	{TOK_USING, "USING", false},
	{TOK_UUID_TO_CHAR, "UUID_TO_CHAR", true},
	{TOK_VALUE, "VALUE", false},
	{TOK_VALUES, "VALUES", false},
	{TOK_VAR_POP, "VAR_POP", false},
	{TOK_VAR_SAMP, "VAR_SAMP", false},
	{TOK_VARBINARY, "VARBINARY", false},
	{TOK_VARCHAR, "VARCHAR", false},
	{TOK_VARIABLE, "VARIABLE", false},
	{TOK_VARYING, "VARYING", false},
	{TOK_VIEW, "VIEW", false},
	{TOK_WAIT, "WAIT", true},
	{TOK_WEEK, "WEEK", true},
	{TOK_WEEKDAY, "WEEKDAY", true},
	{TOK_WHEN, "WHEN", false},
	{TOK_WHERE, "WHERE", false},
	{TOK_WHILE, "WHILE", false},
	{TOK_WINDOW, "WINDOW", false},
	{TOK_WITH, "WITH", false},
	{TOK_WITHOUT, "WITHOUT", false},
	{TOK_WORK, "WORK", true},
	{TOK_WRITE, "WRITE", true},
	{TOK_YEAR, "YEAR", false},
	{TOK_YEARDAY, "YEARDAY", true},
	{TOK_ZONE, "ZONE", true},
	{TOK_NOT_LSS, "^<", false},	// Alias of !<
	{TOK_NEQ, "^=", false},				// Alias of !=
	{TOK_NOT_GTR, "^>", false},			// Alias of !>
	{TOK_CONCATENATE, "||", false},
	{TOK_NOT_LSS, "~<", false},	// Alias of !<
	{TOK_NEQ, "~=", false},				// Alias of !=
	{TOK_NOT_GTR, "~>", false},			// Alias of !>
	{0, NULL, false}
};

Tokens keywordGetTokens()
{
	return tokens;
}
