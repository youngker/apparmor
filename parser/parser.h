/*   $Id$ */

/*
 *   Copyright (c) 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007
 *   NOVELL (All rights reserved)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, contact Novell, Inc.
 */

#include <netinet/in.h>
#include "pcre/internal.h"
#include "immunix.h"
#include "libapparmor_re/apparmor_re.h"

typedef enum pattern_t pattern_t;

struct flagval {
  	int debug;
  	int complain;
  	int audit;
};

struct cod_pattern {
	char *regex;		// posix regex
	pcre *compiled;		// compiled regex, size is compiled->size
};

struct cod_entry {
	char * name ;
	struct codomain *codomain ; 	/* Special codomain defined
					 * just for this executable */
	int mode ;			/* mode is 'or' of AA_* bits */
	int deny ;			/* TRUE or FALSE */

	pattern_t pattern_type;
	struct cod_pattern pat;

	struct cod_entry *next;
};

struct cod_net_entry {
	struct in_addr *saddr, *smask;
	struct in_addr *daddr, *dmask;
	unsigned short src_port[2], dst_port[2];
	char *iface;
	int mode;
	struct cod_net_entry *next;
};

/* supported AF protocols */
struct aa_network_entry {
	unsigned int family;
	unsigned int type;
	unsigned int protocol;

	struct aa_network_entry *next;
};

struct codomain {
	char *name;				/* codomain name */
	char *sub_name;				/* subdomain name or NULL */
	int default_deny;			/* TRUE or FALSE */

	struct codomain *parent;

	struct flagval flags;

	unsigned int capabilities;
	unsigned int *network_allowed;		/* array of type masks
						 * indexed by AF_FAMILY */

	struct cod_entry *entries;
	struct cod_net_entry * net_entries;
	void *hat_table;
	//struct codomain *next;

	aare_ruleset_t *dfarules;
	int dfarule_count;
	void *dfa;
	size_t dfa_size;
} ;

struct cod_global_entry {
	struct cod_entry *entry;
	struct cod_net_entry *net_entry;
	struct codomain *hats ;
	unsigned int capabilities;
};

struct sd_hat {
	char *hat_name;
	unsigned int hat_magic;
};

/* describe an ip address */
struct ipv4_desc {
	struct in_addr addr;
	unsigned long mask;
	unsigned short port[2];
};

struct ipv4_endpoints {
	struct ipv4_desc * src;
	struct ipv4_desc * dest;
};

struct var_string {
	char *prefix;
	char *var;
	char *suffix;
};

#define COD_READ_CHAR 		'r'
#define COD_WRITE_CHAR 		'w'
#define COD_APPEND_CHAR		'a'
#define COD_EXEC_CHAR 		'x'
#define COD_LINK_CHAR 		'l'
#define COD_LOCK_CHAR		'k'
#define COD_MMAP_CHAR		'm'
#define COD_INHERIT_CHAR 	'i'
#define COD_UNCONFINED_CHAR	'U'
#define COD_UNSAFE_UNCONFINED_CHAR	'u'
#define COD_PROFILE_CHAR	'P'
#define COD_UNSAFE_PROFILE_CHAR	'p'

#define OPTION_ADD      1
#define OPTION_REMOVE   2
#define OPTION_REPLACE  3
#define OPTION_STDOUT	4

#define AA_RE_NONE 0
#define AA_RE_PCRE 1
#define AA_RE_DFA 2

#define AA_DIR_AS_NONE 0
#define AA_DIR_AS_FILE 1
#define AA_DIR_AS_DIR  2

#define AA_NET_NONE 0
#define AA_NET_SIMPLE 1

#define AA_SYSCTL_CAP 0
#define AA_SYSCTL_FILE 1

#define AA_EMBED_HATS 0
#define AA_FLATTEN_HATS 1

#define AA_CHANGE_PROFILE_NO 0
#define AA_CHANGE_PROFILE_YES 1

#ifdef DEBUG
#define PDEBUG(fmt, args...) printf("parser: " fmt, ## args)
#else
#define PDEBUG(fmt, args...)	/* Do nothing */
#endif
#define NPDEBUG(fmt, args...)	/* Do nothing */

#define PERROR(fmt, args...) fprintf(stderr, fmt, ## args)

#ifndef TRUE
#define TRUE	(1)
#endif
#ifndef FALSE
#define FALSE	(0)
#endif

#define MIN_PORT 0
#define MAX_PORT 65535

#ifndef __unused
#define __unused __attribute__ ((unused))
#endif

#define list_for_each(LIST, ENTRY) \
	for ((ENTRY) = (LIST); (ENTRY); (ENTRY) = (ENTRY)->next)
#define list_for_each_safe(LIST, ENTRY, TMP) \
	for ((ENTRY) = (LIST), (TMP) = (LIST) ? (LIST)->next : NULL; (ENTRY); (ENTRY) = (TMP), (TMP) = (TMP) ? (TMP)->next : NULL)
#define list_last_entry(LIST, ENTRY) \
	for ((ENTRY) = (LIST); (ENTRY) && (ENTRY)->next; (ENTRY) = (ENTRY)->next)

/* Some external definitions to make b0rken programs happy */
extern char *progname;
extern char *subdomainbase;
extern char *profilename;

/* from parser_main */
extern int force_complain;
extern int regex_type;
extern int aa_valid_file_perms;
extern int aa_dropped_file_perms;
extern int dir_type;
extern int network_type;
extern int sysctl_type;
extern int hat_type;
extern int change_profile_type;

extern void pwarn(char *fmt, ...) __attribute__((__format__(__printf__, 1, 2)));

extern int yyparse(void);
extern void yyerror(char *msg, ...);
extern int yylex(void);

/* parser_regex.c */
extern int process_regex(struct codomain *cod);
extern int post_process_entry(struct cod_entry *entry);

/* parser_variable.c */
extern int process_variables(struct codomain *cod);
extern struct var_string *split_out_var(char *string);
extern void free_var_string(struct var_string *var);

/* parser_misc.c */
extern char *processquoted(char *string, int len);
extern char *processunquoted(char *string, int len);
extern int get_keyword_token(const char *keyword);
extern char *process_var(const char *var);
extern int parse_mode(const char *mode);
extern struct cod_entry *new_entry(char *id, int mode);
extern struct cod_net_entry *new_network_entry(int action,
					       struct ipv4_endpoints *addrs,
					       char *interface);
extern struct aa_network_entry *new_network_ent(unsigned int family,
						unsigned int type,
						unsigned int protocol);
extern struct aa_network_entry *network_entry(const char *family,
					      const char *type,
					      const char *protocol);

extern void debug_cod_list(struct codomain *list);
/* returns -1 if value != true or false, otherwise 0 == false, 1 == true */
extern int str_to_boolean(const char* str);
extern struct cod_entry *copy_cod_entry(struct cod_entry *cod);
extern void free_cod_entries(struct cod_entry *list);
extern void free_net_entries(struct cod_net_entry *list);
extern void free_ipv4_endpoints(struct ipv4_endpoints *addrs);

/* parser_symtab.c */
extern int add_boolean_var(const char *var, int boolean);
extern int get_boolean_var(const char *var);
extern int new_set_var(const char *var, const char *value);
extern int add_set_value(const char *var, const char *value);
extern void *get_set_var(const char *var);
extern char *get_next_set_value(void **context);
extern void dump_symtab(void);
extern void dump_expanded_symtab(void);

/* parser_merge.c */
extern int codomain_merge_rules(struct codomain *cod);

/* parser_interface.c */
typedef struct __sdserialize sd_serialize;
extern int load_codomain(int option, struct codomain *cod);
extern int sd_serialize_profile(sd_serialize *p, struct codomain *cod,
				int flatten);

/* parser_policy.c */
extern void add_to_list(struct codomain *codomain);
extern void add_hat_to_policy(struct codomain *policy, struct codomain *hat);
extern void add_entry_to_policy(struct codomain *policy, struct cod_entry *entry);
extern void add_netrule_to_policy(struct codomain *policy, struct cod_net_entry *net_entry);
extern int post_process_policy(void);
extern int process_hat_regex(struct codomain *cod);
extern int process_hat_variables(struct codomain *cod);
extern int post_merge_rules(void);
extern int merge_hat_rules(struct codomain *cod);
extern struct codomain *merge_policy(struct codomain *a, struct codomain *b);
extern int load_policy(int option);
extern int load_hats(sd_serialize *p, struct codomain *cod);
extern int load_flattened_hats(struct codomain *cod);
extern void free_policy(struct codomain *cod);
extern void dump_policy(void);
extern void dump_policy_hats(struct codomain *cod);
extern void dump_policy_names(void);
extern int die_if_any_regex(void);
