#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <sqlite3.h>
#include "inetfuncs.h"

struct file_list_s {
	struct file_list_s *next;
	FILE *f;
};

int main(int argc, char **argv)
{
	__label__ out_return, out_close;
	int rc;
	char *zErr = NULL;
	int didOpenStdin = 0;
	struct file_list_s *head = NULL;
	struct file_list_s *cur;

	sqlite3 *db = NULL;
	rc = sqlite3_open(":memory:", &db);
	if (rc != SQLITE_OK) {
		zErr = "couldn't open database";
		goto out_return;
	}

	rc = inetfuncs_init(db);
	if (rc != SQLITE_OK) {
		zErr = "couldn't add inetfuncs";
		goto out_return;
	}

	rc = sqlite3_exec(
		db,
		"CREATE TABLE a(ip INT);",
		NULL,
		NULL,
		NULL
	);
	if (rc != SQLITE_OK) {
		zErr = "couldn't create table";
		goto out_return;
	}

	sqlite3_stmt *stmt_insert = NULL;
	rc = sqlite3_prepare_v2(
		db,
		//"INSERT INTO a(ip) VALUES(inet_pton(:ip));",
		"INSERT INTO a(ip) VALUES(:ip);",
		-1,
		&stmt_insert,
		NULL
	);
	if (rc != SQLITE_OK) {
		zErr = "couldn't prepare insert";
		goto out_close;
	}

	sqlite3_stmt *stmt_select = NULL;
	rc = sqlite3_prepare_v2(
		db,
		//"SELECT inet_ntop(ip) FROM a ORDER BY ip;",
		"SELECT ip FROM a ORDER BY inet_pton(ip);",
		-1,
		&stmt_select,
		NULL
	);
	if (rc != SQLITE_OK) {
		zErr = "couldn't prepare select";
		goto out_finalize_insert;
	}

	if (argc < 2) {
		head = malloc(sizeof(struct file_list_s));
		head->f = stdin;
		head->next = NULL;
		didOpenStdin = 1;
	} else {
		int i;
		for (i = 1; i < argc; argc++) {
			struct file_list_s *oldhead = head;
			FILE *f;
			if (!strcmp(argv[i], "-")) {
				if (!didOpenStdin)
					f = stdin;
				didOpenStdin = 1;
			} else {
				f = fopen(argv[i], "r");
			}
			if (!f) {
				zErr = "couldn't open file";
				goto out_finalize_select;
			}
			head = malloc(sizeof(struct file_list_s));
			head->next = oldhead;
			head->f = f;
		}
	}

	size_t lineLen = 16;
	char *line = malloc(lineLen);
	cur = head;
	while (cur) {
		ssize_t chars;
		while((chars = getline(&line, &lineLen, cur->f)) >= 0) {
			rc = sqlite3_bind_text(
				stmt_insert,
				1,
				line,
				chars - 1,
				SQLITE_TRANSIENT
			);
			if (rc != SQLITE_OK) {
				zErr = "in insert bind";
				goto out_finalize_select;
			}
			rc = sqlite3_step(stmt_insert);
			if (rc != SQLITE_DONE) {
				zErr = "in insert step";
				goto out_finalize_select;
			}
			rc = sqlite3_reset(stmt_insert);
			if (rc != SQLITE_OK) {
				zErr = "in insert reset";
				goto out_finalize_select;
			}
		}
		cur = cur->next;
	}
	free(line);
	line = NULL;


	while (SQLITE_DONE != (rc = sqlite3_step(stmt_select))) switch (rc) {
	case SQLITE_ROW:
		{
		const char *s = sqlite3_column_text(stmt_select, 0);
		if (s) printf("%s\n", s);
		}
		break;
	default:
		zErr = "in select step";
		goto out_finalize_select;
		break;
	}

out_finalize_select:
	sqlite3_finalize(stmt_select);
	stmt_select = NULL;
out_finalize_insert:
	sqlite3_finalize(stmt_insert);
	stmt_insert = NULL;
out_close:
	sqlite3_close(db);
	db = NULL;
	cur = head;
	while (cur) {
		struct file_list_s *u = cur->next;
		fclose(cur->f);
		cur->f = NULL;
		cur->next = NULL;
		free(cur);
		cur = u;
	}
out_return:
	if (zErr) {
		fprintf(stderr, "%s: error: %s\n", argv[0], zErr);
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}
