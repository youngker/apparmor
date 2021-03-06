/* $Id$ */

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

#define _GNU_SOURCE    /* for asprintf */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#define _(s) gettext(s)

#include "parser.h"
#include "libapparmor_re/apparmor_re.h"

#include <unistd.h>
#include <linux/unistd.h>

/* only for x86 at the moment */
#include <endian.h>
#include <byteswap.h>
#include <libintl.h>
#define _(s) gettext(s)

#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned int

#define BUFFERINC 65536
//#define BUFFERINC 16

#if __BYTE_ORDER == __BIG_ENDIAN
#  define cpu_to_le16(x) ((u16)(bswap_16 ((u16) x)))
#  define cpu_to_le32(x) ((u32)(bswap_32 ((u32) x)))
#else
#  define cpu_to_le16(x) ((u16)(x))
#  define cpu_to_le32(x) ((u32)(x))
#endif

#define SD_CODE_SIZE (sizeof(u8))
#define SD_STR_LEN (sizeof(u16))

#define SUBDOMAIN_INTERFACE_VERSION 2
#define SUBDOMAIN_INTERFACE_DFA_VERSION 3

int sd_serialize_codomain(int option, struct codomain *cod);

static void print_error(int error)
{
	switch (error) {
	case -ESPIPE:
		PERROR(_("Bad write position\n"));
		break;
	case -EPERM:
		PERROR(_("Permission denied\n"));
		break;
	case -ENOMEM:
		PERROR(_("Out of memory\n"));
		break;
	case -EFAULT:
		PERROR(_("Couldn't copy profile Bad memory address\n"));
		break;
	case -EPROTO:
		PERROR(_("Profile doesn't conform to protocol\n"));
		break;
	case -EBADMSG:
		PERROR(_("Profile does not match signature\n"));
		break;
	case -EPROTONOSUPPORT:
		PERROR(_("Profile version not supported by Apparmor module\n"));
		break;
	case -EEXIST:
		PERROR(_("Profile already exists\n"));
		break;
	case -ENOENT:
		PERROR(_("Profile doesn't exist\n"));
		break;
	default:
		PERROR(_("Unknown error\n"));
		break;
	}
}

int load_codomain(int option, struct codomain *cod)
{
	int retval = 0;
	int error = 0;

	PDEBUG("Serializing policy for %s.\n", cod->name);
	retval = sd_serialize_codomain(option, cod);

	if (retval < 0) {
		error = retval;	/* yeah, we'll just report the last error */
		switch (option) {
		case OPTION_ADD:
			PERROR(_("%s: Unable to add \"%s\".  "),
			       progname, cod->name);
			print_error(error);
			break;
		case OPTION_REPLACE:
			PERROR(_("%s: Unable to replace \"%s\".  "),
			       progname, cod->name);
			print_error(error);
			break;
		case OPTION_REMOVE:
			PERROR(_("%s: Unable to remove \"%s\".  "),
			       progname, cod->name);
			print_error(error);
			break;
		case OPTION_STDOUT:
			PERROR(_("%s: Unable to write to stdout\n"),
			       progname);
			break;
		default:
			PERROR(_("%s: ASSERT: Invalid option: %d\n"),
			       progname, option);
			exit(1);
			break;
		}

	} else {
		switch (option) {
		case OPTION_ADD:
			printf(_("Addition succeeded for \"%s\".\n"),
			       cod->name);
			break;
		case OPTION_REPLACE:
			printf(_("Replacement succeeded for \"%s\".\n"),
			       cod->name);
			break;
		case OPTION_REMOVE:
			printf(_("Removal succeeded for \"%s\".\n"),
			       cod->name);
			break;
		case OPTION_STDOUT:
			break;
		default:
			PERROR(_("%s: ASSERT: Invalid option: %d\n"),
			       progname, option);
			exit(1);
			break;
		}
	}

	return error;
}



enum sd_code {
	SD_U8,
	SD_U16,
	SD_U32,
	SD_U64,
	SD_NAME,		/* same as string except it is items name */
	SD_STRING,
	SD_BLOB,
	SD_STRUCT,
	SD_STRUCTEND,
	SD_LIST,
	SD_LISTEND,
	SD_ARRAY,
	SD_ARRAYEND,
	SD_OFFSET
};

const char *sd_code_names[] = {
	"SD_U8",
	"SD_U16",
	"SD_U32",
	"SD_U64",
	"SD_NAME",
	"SD_STRING",
	"SD_BLOB",
	"SD_STRUCT",
	"SD_STRUCTEND",
	"SD_LIST",
	"SD_LISTEND",
	"SD_OFFSET"
};

/* Currently we will just use a contiguous block of memory
   be we are going to just hide this for the moment.  */
struct __sdserialize {
	void *buffer;
	void *pos;
	void *extent;
};

sd_serialize *alloc_sd_serial(void)
{
	sd_serialize *p = malloc(sizeof(sd_serialize));
	if (!p)
		return NULL;
	p->buffer = malloc(BUFFERINC);
	if (!p->buffer) {
		free(p);
		return NULL;
	}
	p->pos = p->buffer;
	p->extent = p->buffer + BUFFERINC;
	return p;
}

void free_sd_serial(sd_serialize *p)
{
	if (p) {
		if (p->buffer)
			free(p->buffer);
		free(p);
	}
}

/*check if something of size length is in sd_serial bounds */
static inline int sd_inbounds(sd_serialize *p, int size)
{
	return (p->pos + size <= p->extent);
}

static inline void sd_inc(sd_serialize *p, int size)
{
	if (sd_inbounds(p, size)) {
		p->pos += size;
	} else {
		PERROR(_("PANIC bad increment buffer %p pos %p ext %p size %d res %p\n"),
		       p->buffer, p->pos, p->extent, size, p->pos + size);
		exit(-1);
	}
}

inline long sd_serial_size(sd_serialize *p)
{
	return (p->pos - p->buffer);
}

/* routines for writing data to the serialization buffer */
inline int sd_prepare_write(sd_serialize *p, enum sd_code code, size_t size)
{
	int num = (size / BUFFERINC) + 1;
	if (p->pos + SD_CODE_SIZE + size > p->extent) {
		long pos;
		/* try and reallocate the buffer */
		void *buffer = malloc(p->extent - p->buffer + (BUFFERINC * num));
		memcpy(buffer, p->buffer, p->extent - p->buffer);

		pos = p->pos - p->buffer;
		if (buffer == NULL || errno == ENOMEM)
			return 0;

		p->extent = buffer + (p->extent - p->buffer) + (BUFFERINC * num);
		free(p->buffer);
		p->buffer = buffer;
		p->pos = buffer + pos;
	}
	*(u8 *) (p->pos) = code;
	sd_inc(p, SD_CODE_SIZE);
	return 1;
}

inline int sd_write8(sd_serialize *p, u8 b)
{
	u8 *c;
	if (!sd_prepare_write(p, SD_U8, sizeof(b)))
		return 0;
	c = (u8 *) p->pos;
	*c = b;
	sd_inc(p, 1);
	return 1;
}

inline int sd_write16(sd_serialize *p, u16 b)
{
	u16 tmp;
	if (!sd_prepare_write(p, SD_U16, sizeof(b)))
		return 0;
	tmp = cpu_to_le16(b);
	memcpy(p->pos, &tmp, sizeof(tmp));
	sd_inc(p, sizeof(tmp));
	return 1;
}

inline int sd_write32(sd_serialize *p, u32 b)
{
	u32 tmp;
	if (!sd_prepare_write(p, SD_U32, sizeof(b)))
		return 0;
	tmp = cpu_to_le32(b);
	memcpy(p->pos, &tmp, sizeof(tmp));
	sd_inc(p, sizeof(tmp));
	return 1;
}

inline int sd_write_name(sd_serialize *p, char *name)
{
	long size = 0;
	PDEBUG("Writing name '%s'\n", name);
	if (name) {
		u16 tmp;
		size = strlen(name) + 1;
		if (!sd_prepare_write(p, SD_NAME, SD_STR_LEN + size))
			return 0;
		tmp = cpu_to_le16(size);
		memcpy(p->pos, &tmp, sizeof(tmp));
		sd_inc(p, sizeof(tmp));
		memcpy(p->pos, name, size);
		sd_inc(p, size);
	}
	return 1;
}

inline int sd_write_blob(sd_serialize *p, void *b, int buf_size, char *name)
{
	u32 tmp;
	if (!sd_write_name(p, name))
		return 0;
	if (!sd_prepare_write(p, SD_BLOB, 4 + buf_size))
		return 0;
	tmp = cpu_to_le32(buf_size);
	memcpy(p->pos, &tmp, sizeof(tmp));
	sd_inc(p, sizeof(tmp));
	memcpy(p->pos, b, buf_size);
	sd_inc(p, buf_size);
	return 1;
}

#define align64(X) (((size_t) (X) + (size_t) 7) & ~((size_t) 7))
inline int sd_write_aligned_blob(sd_serialize *p, void *b, int buf_size,
				 char *name)
{
	size_t pad;
	u32 tmp;
	if (!sd_write_name(p, name))
		return 0;
	pad = align64((p->pos + 5) - p->buffer) - ((p->pos + 5) - p->buffer);
	if (!sd_prepare_write(p, SD_BLOB, 4 + buf_size + pad))
		return 0;
	tmp = cpu_to_le32(buf_size + pad);
	memcpy(p->pos, &tmp, sizeof(tmp));
	sd_inc(p, sizeof(tmp));
	memset(p->pos, 0, pad);
	sd_inc(p, pad);
	memcpy(p->pos, b, buf_size);
	sd_inc(p, buf_size);
	return 1;
}

inline int sd_write_string(sd_serialize *p, char *b, char *name)
{
	u16 tmp;
	int size;
	if (!sd_write_name(p, name))
		return 0;
	size = strlen(b) + 1;
	if (!sd_prepare_write(p, SD_STRING, SD_STR_LEN + size))
		return 0;
	tmp = cpu_to_le16(size);
	memcpy(p->pos, &tmp, sizeof(tmp));
	sd_inc(p, sizeof(tmp));
	memcpy(p->pos, b, size);
	sd_inc(p, size);
	return 1;
}

inline int sd_write_struct(sd_serialize *p, char *name)
{
	if (!sd_write_name(p, name))
		return 0;
	if (!sd_prepare_write(p, SD_STRUCT, 0))
		return 0;
	return 1;
}

inline int sd_write_structend(sd_serialize *p)
{
	if (!sd_prepare_write(p, SD_STRUCTEND, 0))
		return 0;
	return 1;
}

inline int sd_write_array(sd_serialize *p, char *name, int size)
{
	u16 tmp;
	if (!sd_write_name(p, name))
		return 0;
	if (!sd_prepare_write(p, SD_ARRAY, 2))
		return 0;
	tmp = cpu_to_le16(size);
	memcpy(p->pos, &tmp, sizeof(tmp));
	sd_inc(p, sizeof(tmp));
	return 1;
}

inline int sd_write_arrayend(sd_serialize *p)
{
	if (!sd_prepare_write(p, SD_ARRAYEND, 0))
		return 0;
	return 1;
}

inline int sd_write_list(sd_serialize *p, char *name)
{
	if (!sd_write_name(p, name))
		return 0;
	if (!sd_prepare_write(p, SD_LIST, 0))
		return 0;
	return 1;
}

inline int sd_write_listend(sd_serialize *p)
{
	if (!sd_prepare_write(p, SD_LISTEND, 0))
		return 0;
	return 1;
}

int
sd_serialize_net_entry(sd_serialize *p, struct cod_net_entry *net_entry)
{

	if (!sd_write_struct(p, "ne"))
		return 0;
	if (!sd_write32(p, net_entry->mode))
		return 0;

	if (!sd_write32(p, net_entry->saddr->s_addr))
		return 0;
	if (!sd_write32(p, net_entry->smask->s_addr))
		return 0;
	if (!sd_write16(p, net_entry->src_port[0]))
		return 0;
	if (!sd_write16(p, net_entry->src_port[1]))
		return 0;
	if (!sd_write32(p, net_entry->daddr->s_addr))
		return 0;
	if (!sd_write32(p, net_entry->dmask->s_addr))
		return 0;
	if (!sd_write16(p, net_entry->dst_port[0]))
		return 0;
	if (!sd_write16(p, net_entry->dst_port[1]))
		return 0;

	if (net_entry->iface)
		if (!sd_write_string(p, net_entry->iface, NULL))
			return 0;

	if (!sd_write_structend(p))
		return 0;

	return 1;
}

int sd_serialize_pattern(sd_serialize *p, pcre *pat)
{
	if (!sd_write_struct(p, "pcre"))
		return 0;
	if (!sd_write32(p, pat->size - sizeof(pcre)))
		return 0;
	if (!sd_write32(p, pat->magic_number))
		return 0;
	if (!sd_write32(p, pat->options))
		return 0;
	if (!sd_write16(p, pat->top_bracket))
		return 0;
	if (!sd_write16(p, pat->top_backref))
		return 0;
	if (!sd_write8(p, pat->first_char))
		return 0;
	if (!sd_write8(p, pat->req_char))
		return 0;
	if (!sd_write8(p, pat->code[0]))
		return 0;
	if (!sd_write_blob(p, &pat->code[1], pat->size - sizeof(pcre), NULL))
		return 0;
	if (!sd_write_structend(p))
		return 0;

	return 1;
}

int sd_serialize_file_entry(sd_serialize *p, struct cod_entry *file_entry)
{
	PDEBUG("Writing file entry. name '%s'\n", file_entry->name);
	if (!sd_write_struct(p, "fe"))
		return 0;
	if (!sd_write_string(p, file_entry->name, NULL))
		return 0;
	if (!sd_write32(p, file_entry->mode))
		return 0;
	if (!sd_write32(p, file_entry->pattern_type))
		return 0;
	if (file_entry->pattern_type == ePatternRegex) {
		if (!sd_write_string(p, file_entry->pat.regex, NULL))
			return 0;
		if (!sd_serialize_pattern(p, file_entry->pat.compiled))
			return 0;
	}
	if (!sd_write_structend(p))
		return 0;

	return 1;
}

int sd_serialize_dfa(sd_serialize *p, void *dfa, size_t size)
{
	if (dfa && !sd_write_aligned_blob(p, dfa, size, "aadfa"))
		return 0;

	return 1;
}

int count_file_ents(struct cod_entry *list)
{
	struct cod_entry *entry;
	int count = 0;
	list_for_each(list, entry) {
		if (entry->pattern_type == ePatternBasic) {
			count++;
		}
	}
	return count;
}

int count_tailglob_ents(struct cod_entry *list)
{
	struct cod_entry *entry;
	int count = 0;
	list_for_each(list, entry) {
		if (entry->pattern_type == ePatternTailGlob) {
			count++;
		}
	}
	return count;
}

int count_pcre_ents(struct cod_entry *list)
{
	struct cod_entry *entry;
	int count = 0;
	list_for_each(list, entry) {
		if (entry->pattern_type == ePatternRegex) {
			count++;
		}
	}
	return count;
}

int sd_serialize_profile(sd_serialize *p, struct codomain *profile,
			 int flattened)
{
	struct cod_entry *entry;
	struct cod_net_entry *net_entry;

	if (!sd_write_struct(p, "profile"))
		return 0;
	if (flattened) {
		assert(profile->parent);
		int res;

		char *name = malloc(3 + strlen(profile->name) +
				    strlen(profile->parent->name));
		if (!name)
			return 0;
		sprintf(name, "%s//%s", profile->parent->name, profile->name);
		res = sd_write_string(p, name, NULL);
		free(name);
		if (!res)
			return 0;
	} else {
		if (!sd_write_string(p, profile->name, NULL))
			return 0;
	}
	if (!sd_write_struct(p, "flags"))
		return 0;
	/* used to be flags.debug, but that's no longer supported */
	if (!sd_write32(p, 0))
		return 0;
	if (!sd_write32(p, profile->flags.complain))
		return 0;
	if (!sd_write32(p, profile->flags.audit))
		return 0;
	if (!sd_write_structend(p))
		return 0;
	if (!sd_write32(p, profile->capabilities))
		return 0;

	if (profile->network_allowed && network_type == AA_NET_SIMPLE) {
		int i;
		if (!sd_write_array(p, "net_allowed_af", AF_MAX))
			return 0;
		for (i = 0; i < AF_MAX; i++) {
			if (!sd_write16(p, profile->network_allowed[i]))
				return 0;
		}
		if (!sd_write_arrayend(p))
			return 0;
	} else if (profile->network_allowed && network_type == AA_NET_NONE) {
		PERROR(_("Warning: Ignoring network rules not supported by current AppArmor.\n"));
	}

	/* either have a single dfa or lists of different entry types */
	if (regex_type == AA_RE_DFA) {
		if (aa_dropped_file_perms)
			PERROR(_("Warning: Ignoring filepermissions not supported by current AppArmor.\n"));
		if (!sd_serialize_dfa(p, profile->dfa, profile->dfa_size))
			return 0;
	} else {
		if (aa_dropped_file_perms)
			PERROR(_("Warning: Ignoring filepermissions not supported by current AppArmor.\n"));
		/* pcre globbing entries */
		if (count_pcre_ents(profile->entries)) {
			if (!sd_write_list(p, "pgent"))
				return 0;
			list_for_each(profile->entries, entry) {
				if (entry->pattern_type == ePatternRegex) {
					if (!sd_serialize_file_entry(p, entry))
						return 0;
				}
			}
			if (!sd_write_listend(p))
				return 0;
		}

		/* simple globbing entries */
		if (count_tailglob_ents(profile->entries)) {
			if (!sd_write_list(p, "sgent"))
				return 0;
			list_for_each(profile->entries, entry) {
				if (entry->pattern_type == ePatternTailGlob) {
					if (!sd_serialize_file_entry(p, entry))
						return 0;
				}
			}
			if (!sd_write_listend(p))
				return 0;
		}

		/* basic file entries */
		if (count_file_ents(profile->entries)) {
			if (!sd_write_list(p, "fent"))
				return 0;
			list_for_each(profile->entries, entry) {
				if (entry->pattern_type == ePatternBasic) {
					if (!sd_serialize_file_entry(p, entry))
						return 0;
				}
			}
			if (!sd_write_listend(p))
				return 0;
		}
	}

	if (profile->net_entries && (regex_type != AA_RE_DFA)) {
		if (!sd_write_list(p, "net"))
			return 0;
		list_for_each(profile->net_entries, net_entry) {
			if (!sd_serialize_net_entry(p, net_entry))
				return 0;
		}
		if (!sd_write_listend(p))
			return 0;

	}

	if (profile->hat_table && hat_type == AA_EMBED_HATS) {
		if (!sd_write_list(p, "hats"))
			return 0;
		if (load_hats(p, profile) != 0)
			return 0;
		if (!sd_write_listend(p))
			return 0;

	}
	if (!sd_write_structend(p))
		return 0;

	return 1;
}

int sd_serialize_top_profile(sd_serialize *p, struct codomain *profile)
{
	int version;

	if (regex_type == AA_RE_DFA)
		version = SUBDOMAIN_INTERFACE_DFA_VERSION;
	else
		version = SUBDOMAIN_INTERFACE_VERSION;


	if (!sd_write_name(p, "version"))
		return 0;

	if (!sd_write32(p, version))
		return 0;
	return sd_serialize_profile(p, profile, profile->parent ? 1 : 0);
}

int sd_serialize_codomain(int option, struct codomain *cod)
{
	int fd;
	int error = 0, size, wsize;
	sd_serialize *work_area;
	char *filename = NULL;

	switch (option) {
	case OPTION_ADD:
		asprintf(&filename, "%s/.load", subdomainbase);
		fd = open(filename, O_WRONLY);
		break;
	case OPTION_REPLACE:
		asprintf(&filename, "%s/.replace", subdomainbase);
		fd = open(filename, O_WRONLY);
		break;
	case OPTION_REMOVE:
		asprintf(&filename, "%s/.remove", subdomainbase);
		fd = open(filename, O_WRONLY);
		break;
	case OPTION_STDOUT:
		filename = "stdout";
		fd = dup(1);
		break;
	default:
		error = -EINVAL;
		goto exit;
		break;
	}

	if (fd < 0) {
		PERROR(_("Unable to open %s - %s\n"), filename,
		       strerror(errno));
		error = -errno;
		goto exit;
	}

	if (option != OPTION_STDOUT)
		free(filename);

	if (option == OPTION_REMOVE) {
		char *name;
		if (cod->parent) {
			name = malloc(strlen(cod->name) + 3 +
				      strlen(cod->parent->name));
			if (!name) {
				PERROR(_("Unable to remove ^%s\n"), cod->name);
				error = -errno;
				goto exit;
			}
			sprintf(name, "%s//%s", cod->parent->name, cod->name);
		} else {
			name = cod->name;
		}
		size = strlen(name) + 1;
		wsize = write(fd, name, size);
		if (wsize < 0)
			error = -errno;
		if (cod->parent)
			free(name);
	} else {

		work_area = alloc_sd_serial();
		if (!work_area) {
			close(fd);
			PERROR(_("unable to create work area\n"));
			error = -ENOMEM;
			goto exit;
		}

		if (!sd_serialize_top_profile(work_area, cod)) {
			close(fd);
			free_sd_serial(work_area);
			PERROR(_("unable to serialize profile %s\n"),
			       cod->name);
			goto exit;
		}

		size = work_area->pos - work_area->buffer;
		wsize = write(fd, work_area->buffer, size);
		if (wsize < 0) {
			error = -errno;
		} else if (wsize < size) {
			PERROR(_("%s: Unable to write entire profile entry\n"),
			       progname);
		}
		free_sd_serial(work_area);
	}

	close(fd);

	if (cod->hat_table && hat_type == AA_FLATTEN_HATS) {
		if (load_flattened_hats(cod) != 0)
			return 0;
	}


exit:
	return error;
}
