/*
 *   Copyright (c) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
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


#ifndef __AA_LOG_PARSER_H__
#define __AA_LOG_PARSER_H__

extern void _init_log_record(aa_log_record *record);
extern aa_log_record *_parse_yacc(char *str);
extern char *hex_to_string(char *str);

/* FIXME: this ought to be pulled from <linux/audit.h> but there's no
 * guarantee these will exist there. */
#define AUDIT_APPARMOR_AUDIT    1501    /* AppArmor audited grants */
#define AUDIT_APPARMOR_ALLOWED  1502    /* Allowed Access for learning */
#define AUDIT_APPARMOR_DENIED   1503
#define AUDIT_APPARMOR_HINT     1504    /* Process Tracking information */
#define AUDIT_APPARMOR_STATUS   1505    /* Changes in config */
#define AUDIT_APPARMOR_ERROR    1506    /* Internal AppArmor Errors */

#endif

