#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "aalogparse.h"

int print_results(aa_log_record *record);

int main(int argc, char **argv)
{
	FILE *testcase;
	char log_line[1024];
	aa_log_record *test = NULL;
	int ret = -1;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: test_multi.multi <filename>\n");
		return(1);
	}

	printf("START\n");
	printf("File: %s\n", argv[1]);

	testcase = fopen(argv[1], "r");
	if (testcase == NULL)
	{
		perror("Could not open testcase: ");
		return(1);
	}

	if (fgets(log_line, 1023, testcase) == NULL)
	{
		fprintf(stderr, "Could not read testcase.\n");
		fclose(testcase);
		return(1);
	}

	fclose(testcase);

	test = parse_record(log_line);

	if (test == NULL)
	{
		fprintf(stderr,"Parsing failed.\n");
		return(1);
	}
	ret = print_results(test);
	free_record(test);
	return ret;
}

int print_results(aa_log_record *record)
{
		printf("Event type: ");
		switch(record->event)
		{
			case AA_RECORD_ERROR:
			{
				printf("AA_RECORD_ERROR\n");
				break;
			}
			case AA_RECORD_INVALID:
			{
				printf("AA_RECORD_INVALID\n");
				break;
			}
			case AA_RECORD_AUDIT:
			{
				printf("AA_RECORD_AUDIT\n");
				break;
			}
			case AA_RECORD_ALLOWED:
			{
				printf("AA_RECORD_ALLOWED\n");
				break;
			}
			case AA_RECORD_DENIED:
			{
				printf("AA_RECORD_DENIED\n");
				break;
			}
			case AA_RECORD_HINT:
			{
				printf("AA_RECORD_HINT\n");
				break;
			}
			case AA_RECORD_STATUS:
			{
				printf("AA_RECORD_STATUS\n");
				break;
			}
			default:
			{
				printf("UNKNOWN EVENT TYPE\n");
				break;
			}
		}
		if (record->audit_id != NULL)
		{
			printf("Audit ID: %s\n", record->audit_id);
		}
		if (record->operation != NULL)
		{
			printf("Operation: %s\n", record->operation);
		}
		if (record->requested_mask != NULL)
		{
			printf("Mask: %s\n", record->requested_mask);
		}
		if (record->denied_mask != NULL)
		{
			printf("Denied Mask: %s\n", record->denied_mask);
		}
		if (record->profile != NULL)
		{
			printf("Profile: %s\n", record->profile);
		}
		if (record->name != NULL)
		{
			printf("Name: %s\n", record->name);
		}
		if (record->name2 != NULL)
		{
			printf("Name2: %s\n", record->name2);
		}
		if (record->attribute != NULL)
		{
			printf("Attribute: %s\n", record->attribute);
		}
		if (record->task != 0)
		{
			printf("Task: %ld\n", record->task);
		}
		if (record->parent != 0)
		{
			printf("Parent: %ld\n", record->parent);
		}
		if (record->magic_token != 0)
		{
			printf("Token: %lu\n", record->magic_token);
		}
		if (record->info != NULL)
		{
			printf("Info: %s\n", record->info);
		}
		if (record->pid != 0)
		{
			printf("PID: %ld\n", record->pid);
		}
		if (record->active_hat != NULL)
		{
			printf("Active hat: %s\n", record->active_hat);
		}
		if (record->net_family != NULL)
		{
			printf("Network family: %s\n", record->net_family);
		}
		if (record->net_sock_type != NULL)
		{
			printf("Socket type: %s\n", record->net_sock_type);
		}
		if (record->net_protocol != NULL)
		{
			printf("Protocol: %s\n", record->net_protocol);
		}
		printf("Epoch: %lu\n", record->epoch);
		printf("Audit subid: %u\n", record->audit_sub_id);
	return(0);
}
