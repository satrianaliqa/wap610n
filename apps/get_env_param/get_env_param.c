/*  get_env_param - read u-boot params from linux
 *
 *  Copyright (C) 2008 Metalink Ltd.
 *
 *  Parts of this code were based on srcs from u-boot-1.1.4:
 *        (C) Copyright 2000 - 2005
 *        Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 *  Licensed under GPLv2 or later, see file LICENSE 
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char* argv[])
{
    FILE *f;
    uint32_t r;
	char *param_name, curr_param[256], param_value[256];
	int curr_char;
	int curr_idx=0, value_idx=0, reading_value=0;
	int name_truncated=0, value_truncated=0;
	
	if (argc !=2)
    	return 1;

	param_name = argv[1];
	if (param_name[0] == '\0' || strchr(param_name, '=') != NULL)
		return 1;

    f=fopen("/dev/mtdblock3","r");
    if (!f)
	{
        return 1;
	}

	/* Skip the 4 bytes of crc */
    if (fread(&r, sizeof(uint32_t), 1, f) != 1)
	{
		fclose(f);
		return 1;
	}

	curr_param[0] = param_value[0] = 0;
	while ((curr_char = fgetc(f)) != EOF)
	{
		if (curr_char == '=')
		{
			if (!reading_value)
				reading_value = 1;
			continue;
		}
		if (curr_char == 0)
		{
			if (reading_value && !name_truncated && !value_truncated &&
				!strcmp(curr_param, param_name))
			{
				printf("%s\n", param_value); //Found param 
				fclose(f);
				return 0;
			}
			curr_idx = 0;
			value_idx = 0;
			reading_value = 0;
			name_truncated = 0;
			value_truncated = 0;
			curr_param[0] = param_value[0] = 0;
			continue;
		}
		if (reading_value)
		{
			if (value_idx < (int)sizeof(param_value) - 1)
				param_value[value_idx++] = (char)curr_char;
			else
				value_truncated = 1;
			param_value[value_idx] = 0;
		}
		else
		{
			if (curr_idx < (int)sizeof(curr_param) - 1)
				curr_param[curr_idx++] = (char)curr_char;
			else
				name_truncated = 1;
			curr_param[curr_idx] = 0;
		}
	}
    fclose(f);
    return 1;
}

