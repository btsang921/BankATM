#include "atm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>


void make_bank_happy(ATM *atm) {
	char *fake_msg = "hello";
	atm_send(atm, fake_msg, strlen(fake_msg) + 1);
	char fake_response[10];
	atm_recv(atm, fake_response, sizeof(fake_response));
}


// Default port and ip address are defined here

int main(int argc, char** argv){
  	

	regex_t ip_regex;
	char* ip_pattern = "^(([0-1]?[0-9]?[0-9]?|2[0-4][0-9]|25[0-5])\\.){3}([0-1]?[0-9]?[0-9]?|2[0-4][0-9]|25[0-5]){1}$";
	regcomp(&ip_regex, ip_pattern, REG_EXTENDED);

	regex_t filename_regex;
	char * filename_pattern = "^[0-9a-z._-]{1,127}$";
	regcomp(&filename_regex, filename_pattern, REG_EXTENDED);

	regex_t number_regex;
	char *number_pattern = "^(0|[1-9][0-9]*)$";
	regcomp(&number_regex, number_pattern, REG_EXTENDED);

	regex_t balances_regex;
	char *balances_pattern = "^(0.[0-9]{2})|^([1-9][0-9]*.[0-9]{2})$";
	regcomp(&balances_regex, balances_pattern, REG_EXTENDED);

	regex_t accounts_regex;
	char *accounts_pattern = "^[0-9a-z._-]{1,122}$";
	regcomp(&accounts_regex, accounts_pattern, REG_EXTENDED);

	int c;

	int n_mode = 0;
	int d_mode = 0;
	int w_mode = 0;
	int g_mode = 0;
	int has_acct = 0;
	int s_flag = 0;
	int i_flag = 0;
	int p_flag = 0;
	int c_flag = 0;

	char *n_value = NULL;
	char *d_value = NULL;
	char *w_value = NULL;
	char *acct_name = NULL;
	char *s_value = "bank.auth"; // default
	char *ip_addr = "127.0.0.1"; // default
	unsigned short port = 3000; // default
	char *c_value = NULL; // default is <acct_name>.card, gets set below if c flag isnt in args
	

	int old_optind = optind;

	while ((c = getopt(argc, argv, "n:d:w:ga:s:i:p:c:")) != -1) {
		if (old_optind + 2 < optind) {
			exit(255);
		}
		old_optind = optind;
		switch (c) {
			case 'n':
				if (n_mode) {
					exit(255);
				}
				n_mode = 1;
				n_value = optarg;
				if (regexec(&balances_regex, n_value, 0, NULL, 0) != 0) {
					exit(255);
				}
				break;
			case 'd':
				if (d_mode) {
					exit(255);
				}	
				d_mode = 1;
				d_value = optarg;
				if (regexec(&balances_regex, d_value, 0, NULL, 0) != 0) {
					exit(255);
				}
				break;
			case 'w':
				if (w_mode) {
					exit(255);
				}
				w_mode = 1;
				w_value = optarg;
				if (regexec(&balances_regex, w_value, 0, NULL, 0) != 0) {
					exit(255);
				}
				break;
			case 'g':
				if (g_mode) {
					exit(255);
				}
				g_mode = 1;
				break;
			case 'a':
				if (has_acct) {
					exit(255);
				}
				has_acct = 1;
				acct_name = optarg;
				if (regexec(&accounts_regex, acct_name, 0, NULL, 0) != 0){
					exit(255);
				}
				break;
			case 's':
				if (s_flag) {
					exit(255);
				}
				s_flag = 1;
				s_value = optarg;
				if (strcmp(s_value, ".") == 0 || strcmp(s_value, "..") == 0) {
					exit(255);
				}
				if (regexec(&filename_regex, s_value, 0, NULL, 0) != 0) {
					exit(255);
				}
				break;
			case 'i':
				if (i_flag) {
					exit(255);
				}
				i_flag = 1;
				ip_addr = optarg;
				if (regexec(&ip_regex, ip_addr, 0, NULL, 0) != 0) {
					exit(255);
				}
				break;
			case 'p':
				if (p_flag) {
					exit(255);
				}
				p_flag = 1;
				port = (unsigned short) atoi(optarg);
				if (port < 1024 || port > 65535) {
					exit(255);
				}
				break;
			case 'c':
				if (c_flag) {
					exit(255);
				}
				c_flag = 1;
				c_value = optarg;
				if (regexec(&filename_regex, c_value, 0, NULL, 0) != 0) {
					exit(255);
				}
				break;
			case '?':
				exit(255);
			default:
				exit(255);
			}

	}

	if (optind < argc) {
		exit(255);
	}


	// if doesn't have account name  or was missing a mode/was given more than 1 mode
	if (!has_acct || !(n_mode ^ d_mode ^ w_mode ^ g_mode)) {
		exit(255);
	}

	if (!c_flag) {
		c_value = malloc(strlen(acct_name) + 6); // space for <account>.card
		if (c_value == NULL) {
			exit (255);
		}
		strncpy(c_value, acct_name, strlen(acct_name) + 1);
		strcat(c_value, ".card");
	}

	// if the auth file does not exist
	if (access(s_value, F_OK) != 0) {
		exit(255);
	}


	ATM *atm = atm_create(ip_addr, port);


	if (n_mode) {


		double starting_bal = atof(n_value);
		if (starting_bal < 10 || starting_bal > 4294967295.99) {
			make_bank_happy(atm);
			exit(255);
		}

		// if the card file already exists
		if (access(c_value, F_OK) == 0) {
			make_bank_happy(atm);
			exit(255);
		}

		char *msg = malloc(2 + strlen(acct_name) + 1 + strlen(n_value) + 1); // <mode>,<acct_name>,<initial_bal>
		sprintf(msg, "n,%s,%s", acct_name, n_value);

		atm_send(atm, msg, strlen(msg) + 1);
		char response[100]; // response will either be "success:pin" or "error"
		atm_recv(atm, response, sizeof(response));

		if (strstr(response, "success") == response) { // success should be first thing in response (success:<pin>)
			// make card file
			FILE *fp = fopen(c_value, "w");

			printf("{\"account\":\"%s\",\"initial_balance\":%s}\n", acct_name, n_value);
			fflush(stdout);

			// get the pin from the bank's response to put in the card file
			char pin_string[33];
			memcpy(pin_string, response + 8, 32);
			pin_string[32] = '\0'; // null terminate
			fprintf(fp, "%s", pin_string);
		}
		else {
			exit(255);
		}
	} else if (d_mode) {
		// if the card file does not exist
		if (access(c_value, F_OK) != 0) {
			make_bank_happy(atm);
			exit(255);
		}

		double deposit_amt = atof(d_value);
		if (deposit_amt <= 0 || deposit_amt > 4294967295.99) {
			make_bank_happy(atm);
			exit(255);
		}

		int size_of_msg = 2 + 32 + 1 + strlen(acct_name) + 1 + strlen(d_value) + 1;
		char *msg = malloc(size_of_msg); // <mode>,<pin>,<acct_name>,<deposit_amt>

		FILE *card_file = fopen(c_value, "r");

		if (card_file == NULL) {
			make_bank_happy(atm);
			exit(255);
		}
		char pin_from_file[32];
		
		if (fread(pin_from_file, 32, 1, card_file) != 1) {
			make_bank_happy(atm);
			exit(255);
		}

	    sprintf(msg, "d,");
		memcpy(msg + 2, pin_from_file, 32);
		sprintf(msg + 34, ",%s,%s", acct_name, d_value);

		atm_send(atm, msg, size_of_msg);
		char response[8]; // response will either be "success" or "error"
		atm_recv(atm, response, sizeof(response));
		if (strcmp(response, "success") == 0) {
			printf("{\"account\":\"%s\",\"deposit\":%s}\n", acct_name, d_value);
			fflush(stdout);
		} else{
			exit(255);
		}
	} else if (w_mode) {
		// if the card file does not exist
		if (access(c_value, F_OK) != 0) {
			make_bank_happy(atm);
			exit(255);
		}

		double withdraw_amt = atof(w_value);
		if (withdraw_amt <= 0 || withdraw_amt > 4294967295.99) {
			make_bank_happy(atm);
			exit(255);
		}

		int size_of_msg = 2 + 32 + 1 + strlen(acct_name) + 1 + strlen(w_value) + 1;
		char *msg = malloc(size_of_msg); // <mode>,<pin>,<acct_name>,<withdraw_amt>

		FILE *card_file = fopen(c_value, "r");

		if (card_file == NULL) {
			make_bank_happy(atm);
			exit(255);
		}
		char pin_from_file[32];

		if (fread(pin_from_file, 32, 1, card_file) != 1) {
			make_bank_happy(atm);
			exit(255);
		}

		sprintf(msg, "w,");
		memcpy(msg + 2, pin_from_file, 32);
		sprintf(msg + 34, ",%s,%s", acct_name, w_value);

		atm_send(atm, msg, size_of_msg);
		char response[8]; // response will either be "success" or "error"
		atm_recv(atm, response, sizeof(response));
		if (strcmp(response, "success") == 0) {
			printf("{\"account\":\"%s\",\"withdraw\":%s}\n", acct_name, w_value);
			fflush(stdout);
		}
		else {
			exit(255);
		}
	} else if (g_mode) {
		// if the card file does not exist
		if (access(c_value, F_OK) != 0) {
			make_bank_happy(atm);
			exit(255);
		}

		int size_of_msg = 2 + 32 + 1 + strlen(acct_name) + 1;
		char *msg = malloc(size_of_msg); // <mode>,<pin>,<acct_name>

		FILE *card_file = fopen(c_value, "r");

		if (card_file == NULL) {
			make_bank_happy(atm);
			exit(255);
		}
		char pin_from_file[32];

		if (fread(pin_from_file, 32, 1, card_file) != 1) {
			make_bank_happy(atm);
			exit(255);
		}

		sprintf(msg, "g,");
		memcpy(msg + 2, pin_from_file, 32);
		sprintf(msg + 34, ",%s", acct_name);

		atm_send(atm, msg, size_of_msg);
		char response[100]; // response will either be <current balance> or "error"
		atm_recv(atm, response, sizeof(response));
		if (strcmp(response, "error") == 0){
			exit(255);
		} else {
			printf("{\"account\":\"%s\",\"balance\":%s}\n", acct_name, response);
			fflush(stdout);
		}
	} else { // somehow not one of these modes?
		exit(255);
	}

	/* send messages */

	/*char buffer[] = "Hello I am the atm and I would like to have money please";
	atm_send(atm, buffer, sizeof(buffer));
	atm_recv(atm, buffer, sizeof(buffer));
	printf("atm received %s\n", buffer);*/
	
	atm_free(atm);
	


	// Implement how atm protocol will work: sanitizing inputs and using different modes of operations

	return EXIT_SUCCESS;
}
