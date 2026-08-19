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
	char *param_name, curr_param[256], param_value[256], *curr_str = curr_param;
	int curr_char;
	int curr_idx=0;
	
	if (argc !=2)
    	return 1;

	param_name = argv[1];

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
		/* Read the param string in format name=value*/
		if (curr_idx < 255)
			curr_str[curr_idx++] = (char)curr_char;
		if (curr_char=='=') // End of name, move to read the value
		{
			curr_str[curr_idx-1] = 0;
			curr_idx = 0;
			curr_str = param_value;
			continue;
		}
		if (curr_char==0) // End of name=value string
		{
			curr_idx = 0;
			curr_str = curr_param;
			if (!strcmp(curr_param, param_name))
			{
				printf("%s\n", param_value); //Found param 
				fclose(f);
				return 0;
			}
			curr_param[0] = param_value[0] = 0;
		}
	}
    fclose(f);
    return 1;
}

