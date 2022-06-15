#include "bank.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <openssl/rand.h>
#include <time.h>

// Default port and ip address are defined here

typedef struct node {
	double bal; 
	char *acct_name;
	char pin[32];
	struct node *next;
} Node;

Node *head = NULL;

void add_account(char *acct_name, double bal, char pin[]) {
	
	Node *new_node = malloc(sizeof(*new_node));
	new_node -> acct_name = malloc(strlen(acct_name) + 1);
	new_node -> bal = bal;
	
	strncpy(new_node -> acct_name, acct_name, strlen(acct_name) + 1);
	memcpy(new_node -> pin, pin, 32);

	new_node -> next = head;
	head = new_node;
}

Node* find_account(char *acct_name) {
	
	Node *curr = head;
	while (curr != NULL) {
		
		if (strcmp(curr -> acct_name, acct_name) == 0) {
			return curr;
		}
		curr = curr -> next;
	}

	return NULL;
}

void gen_random(char *s, const int len) {
    char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
    srand(time(NULL));
    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

int main(int argc, char** argv){

	regex_t filename_regex;
	char *filename_pattern = "^[0-9a-z._-]{1,127}$";
	regcomp(&filename_regex, filename_pattern, REG_EXTENDED);

	unsigned short port = 3000; // default
	char *ip = "127.0.0.1";
	char *auth_file = "bank.auth"; // default

	int c;
	int pflag = 0;
	int sflag = 0;

	int old_optind = optind;

	while ((c = getopt(argc, argv, "p:s:")) != -1) {
		if (old_optind + 2 < optind) {
			exit(255);
		}
		old_optind = optind;
		switch (c) {
		case 'p':
			if (pflag) {
				exit(255);
			}
			pflag = 1;
			port = (unsigned short)atoi(optarg);
			if (port < 1024 || port > 65535) {
				exit(255);
			}
			break;
		case 's':
			if (sflag) {
				exit(255);
			}
			sflag = 1;
			auth_file = optarg;
			if (regexec(&filename_regex, auth_file, 0, NULL, 0) != 0) {
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

	if (access(auth_file, F_OK) == 0) {
		exit(255);
	} else { //create the auth file
		FILE *auth_f;
		auth_f = fopen(auth_file, "w");
		fwrite("botched", 8, 1, auth_f);
		fclose(auth_f);
		
	}
	printf("created\n");
	fflush(stdout);
	

	/* no error checking is done on any of this. may need to modify this */
	Bank *b = bank_create(auth_file, ip, port);


	/* process each incoming client, one at a time, until complete */
	for(;;) {

		unsigned int len = sizeof(b->remote_addr);
		b->clientfd = accept(b->sockfd, (struct sockaddr*)&b->remote_addr, &len);
		if (b->clientfd < 0) {
			perror("error on accept call");
			exit(255);
		}

		/* okay, connected to bank/atm. Send/recv messages to/from the bank/atm. */
		char buffer[1024];
		bank_recv(b, buffer, sizeof(buffer));

		switch (buffer[0]) { // mode is always first character of message
			case 'n':
			{
				char *acct_name_end = strstr(buffer + 2, ",");
				char *acct_name = malloc(acct_name_end - (buffer + 2) + 1);
				strncpy(acct_name, buffer + 2, acct_name_end - (buffer + 2));
				acct_name[acct_name_end - (buffer + 2)] = '\0';

				char *starting_bal = malloc(strlen(acct_name_end + 1));
				strcpy(starting_bal, acct_name_end + 1);

				

				double starting_balance = atof(starting_bal);

				Node *account = find_account(acct_name);

				if (account == NULL) { // if account doesn't exist yet
					char created_pin[33];

					gen_random(created_pin, 32);
						
					add_account(acct_name, starting_balance, created_pin);
					
					strcpy(buffer, "success:");
					memcpy(buffer + 8, created_pin, 32);
					printf("{\"account\":\"%s\",\"initial_balance\":%s}\n", acct_name, starting_bal);
					fflush(stdout);
					bank_send(b, buffer, 40);
				} else {
					strcpy(buffer, "error");
					bank_send(b, buffer, 6);
				}
					
				break;
			}
			case 'd':
			{
				char pin_from_msg[32];
				memcpy(pin_from_msg, buffer + 2, 32);

				char *acct_name_end = strstr(buffer + 35, ",");
				char *acct_name = malloc(acct_name_end - (buffer + 35) + 1);
				strncpy(acct_name, buffer + 35, acct_name_end - (buffer + 35));
				acct_name[acct_name_end - (buffer + 35)] = '\0';

				char *deposit_amt = malloc(strlen(acct_name_end + 1));
				strcpy(deposit_amt, acct_name_end + 1);

				double deposit_amount = atof(deposit_amt);
				Node *account = find_account(acct_name);

				if (account != NULL) { // account exists
					//check pin
					if (memcmp(account -> pin, pin_from_msg, 32) != 0) {
						
						strcpy(buffer, "error");
						bank_send(b, buffer, 6);
					} else {
						// at this point we know the pin matches
						if (account->bal + deposit_amount < account->bal) { // integer overflow
							strcpy(buffer, "error");
							bank_send(b, buffer, 6);
						} else {
							account->bal = (account->bal) + deposit_amount;
							printf("{\"account\":\"%s\",\"deposit\":%s}\n", acct_name, deposit_amt);
							fflush(stdout);
							strcpy(buffer, "success");
							bank_send(b, buffer, 8);
						}
					}
					
				} else {
					
					strcpy(buffer, "error");
					bank_send(b, buffer, 6);
				}

				break;
			}
			case 'w':
			{
				char pin_from_msg[32];
				memcpy(pin_from_msg, buffer + 2, 32);

				char *acct_name_end = strstr(buffer + 35, ",");
				char *acct_name = malloc(acct_name_end - (buffer + 35) + 1);
				strncpy(acct_name, buffer + 35, acct_name_end - (buffer + 35));
				acct_name[acct_name_end - (buffer + 35)] = '\0';

				char *withdraw_amt = malloc(strlen(acct_name_end + 1));
				strcpy(withdraw_amt, acct_name_end + 1);

				double withdraw_amount = atof(withdraw_amt);
				Node *account = find_account(acct_name);

				if (account != NULL) { // account exists
					//check pin
					if (memcmp(account->pin, pin_from_msg, 32) != 0) {
						
						strcpy(buffer, "error");
						bank_send(b, buffer, 6);
					} else {
						// at this point we know the pin matches
						if (account->bal - withdraw_amount < 0) { // bal would be negative
							
							strcpy(buffer, "error");
							bank_send(b, buffer, 6);
						} else {
							account->bal = (account->bal) - withdraw_amount;
							printf("{\"account\":\"%s\",\"withdraw\":%s}\n", acct_name, withdraw_amt);
							fflush(stdout);
							strcpy(buffer, "success");
							bank_send(b, buffer, 8);
						}
					}
					
				} else {
					
					strcpy(buffer, "error");
					bank_send(b, buffer, 6);
				}
				break;
			}
			case 'g':
			{
				char pin_from_msg[32];
				memcpy(pin_from_msg, buffer + 2, 32);

				char *acct_name = malloc(strlen(buffer + 35));
				strcpy(acct_name, buffer + 35);

				Node *account = find_account(acct_name);

				if (account != NULL) { // account exists
					//check pin
					if (memcmp(account->pin, pin_from_msg, 32) != 0) {
						strcpy(buffer, "error");
						bank_send(b, buffer, 6);
					} else {
						// at this point we know the pin matches
						printf("{\"account\":\"%s\",\"balance\":%.2lf}\n", acct_name, account -> bal);
						fflush(stdout);
						sprintf(buffer, "%.2lf", account->bal);


						
						fflush(stdout);
						bank_send(b, buffer, strlen(buffer) + 1);
					}
		
				} else {
					strcpy(buffer, "error");
					bank_send(b, buffer, 6);
				}
				break;
			}
			default: {
				char *msg = "dud";
				bank_send(b, msg, 4);
				break;
			}
		}
		
		// Implement how atm protocol will work: sanitizing inputs and using different modes of operations
	/* when finished processing commands ...*/
		close(b->clientfd);
		b->clientfd = -1;
		
	}


	
	
	
	return EXIT_SUCCESS;
}
