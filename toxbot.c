#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tox/tox.h>

#define SLEEP_TIME_ISNOTCONNECTED 100000
#define SLEEP_TIME_MAINLOOP 10000
#define BOOTSTRAP_ADDRESS "192.254.75.98"
#define BOOTSTRAP_PORT 33445
#define BOOTSTRAP_KEY "951C88B7E75C867418ACDB5D273821372BB5BD652740BCDF623A4FA293E75D2F"

#define MY_NAME "Toxbot"
#define STATUS_MSG "You know what you have to do!"
#define SAVEFILE "savetox.bin"

#define MSG_INVITE "invite"
#define RETURN_MSG_INVITE "Check, I have invited you!"
#define RETURN_MSG_NOTINVITE "Dude, dunno what you want..."
#define GROUPCHAT_NUMBER 0

/* CONVERT HEX TO BINARY */

char
*hex_string_to_bin(const char *hex_string)
{
    size_t len = strlen(hex_string);
    char *val = malloc(len);

    if (val == NULL)
        printf("failed in hex_string_to_bin");

    size_t i;

    for (i = 0; i < len; ++i, hex_string += 2)
        sscanf(hex_string, "%2hhx", &val[i]);

    return val;
}

/* GET MY ID */

void
get_myid(Tox *m)
{
    char id[TOX_FRIEND_ADDRESS_SIZE * 2 + 1] = {0};
    const char address[TOX_FRIEND_ADDRESS_SIZE];
    
    tox_get_address(m, (uint8_t *) address);
    id_from_data(address, id);
    
    printf("My ID: %s\n",id);
}

/* CONVERT DATA TO HEX ID */

void
id_from_data(const uint8_t *address, uint8_t *id)
{
    size_t i;

    for (i = 0; i < TOX_FRIEND_ADDRESS_SIZE; ++i) {
        char xx[3];
        snprintf(xx, sizeof(xx), "%02X", address[i] & 0xff);
        strcat(id, xx);
    }
}

/* STORE TOX DATA */

int
store_data(Tox *m)
{
    int len = tox_size(m);
    char *buf = malloc(len);

    tox_save(m, (uint8_t *) buf);

    FILE *fd = fopen(SAVEFILE, "wb");
    fwrite(buf, len, 1, fd);

    free(buf);
    fclose(fd);
    return 0;
}

/* LOAD TOX DATA */

int
load_data(Tox *m)
{
    FILE *fd;

    if ((fd = fopen(SAVEFILE, "rb")) != NULL) {
        fseek(fd, 0, SEEK_END);
        int len = ftell(fd);
        fseek(fd, 0, SEEK_SET);

        char *buf = malloc(len);
        fread(buf, len, 1, fd);
        int load_status;
        load_status = tox_load(m, (uint8_t *) buf, len);
        if(load_status==0){
			printf("Loaded savefile successfully.\n");
		}
        if(load_status==-1){
			printf("Failed to load savefile.\n");
			return -1;
		}

        free(buf);
        fclose(fd);
        return 0;
    }else{
		printf("No savefile found, writing new one.\n");
        store_data(m);
        return 1;
    }
}

/* CALLBACK: ON REQUEST */

void
on_request(Tox *m, const uint8_t *public_key, const uint8_t *data, uint16_t length, void *userdata){
	printf("Got friend request.\n");
	
	// Get friend id and send message
	char id[TOX_FRIEND_ADDRESS_SIZE * 2 + 1] = {0};
    char address[TOX_FRIEND_ADDRESS_SIZE];
    
    id_from_data(public_key, id);
    
    printf("Friend ID [Msg]: %s [%s]\n",id,data);
    
	// Answer friend request positive
	char key_answer[TOX_CLIENT_ID_SIZE];
	memcpy(key_answer, public_key, TOX_CLIENT_ID_SIZE);
	
	int friendnumber;
	friendnumber = tox_add_friend_norequest(m, key_answer);
	
	printf("Friend accepted, Friendnumber: %i\n",friendnumber);
}

/* CALLBACK: ON MESSAGE */

void
on_message(Tox *m, int32_t friendnumber, const uint8_t *string, uint16_t length, void *userdata){
	printf("Msg [%i]: %s\n",friendnumber,string);
	uint8_t *msg;
	msg = MSG_INVITE;
	if(!memcmp(msg,string,length*sizeof(uint8_t))){
		tox_invite_friend(m,friendnumber,GROUPCHAT_NUMBER);
		tox_send_message(m,friendnumber,RETURN_MSG_INVITE,strlen(RETURN_MSG_INVITE));
		printf("Invited [%i] to groupchat.\n",friendnumber);
	}
	else{
		tox_send_message(m,friendnumber,RETURN_MSG_NOTINVITE,strlen(RETURN_MSG_NOTINVITE));
		printf("Unknown command from [%i].\n",friendnumber);
	}
}

/* CALLBACK: ON CONNECTION STATUS CHANGE */

void
on_connection_status(Tox *m, int32_t friendnumber, uint8_t status, void *userdata){
	if(status){
		printf("Friend %i gone offline.\n",friendnumber);
	}
	else{
		printf("Friend %i comes online.\n",friendnumber);
	}
}

/* INIT TOX */

static
Tox *init_tox(void)
{
	// Set tox option
	Tox_Options tox_opts;
    tox_opts.ipv6enabled = 0;
    tox_opts.udp_disabled = 0;
    tox_opts.proxy_enabled = 0;
    
    // Init core
    Tox *m = tox_new(&tox_opts);
    
    // Register callback
    tox_callback_friend_request(m, on_request, NULL);
    tox_callback_friend_message(m, on_message, NULL);
    tox_callback_connection_status(m, on_connection_status, NULL);
    
    // Return tox object
    return m;
}

/* MAIN */

int
main(int argc, const char *argv[]){
	// Start toxbot
	printf("// TOXBOT\n");
	
	// Convert public keyto binary data
    char *pub_key = hex_string_to_bin(BOOTSTRAP_KEY);
    
    // Setup mytox
    Tox *m = init_tox();
    int load_status;
    load_status = load_data(m);
    if(load_status == -1) return 1;
    
    tox_set_name(m, (uint8_t *) MY_NAME, strlen(MY_NAME)); // set name
    tox_set_status_message(m,STATUS_MSG,strlen(STATUS_MSG)); // set status
    
    // Init connection
    int bootstrap;
    int i;
	bootstrap = tox_bootstrap_from_address(m, BOOTSTRAP_ADDRESS, BOOTSTRAP_PORT, (uint8_t *) pub_key);
	if(bootstrap){
		printf("Boostrap done.\n");
	}
	else{
		printf("Boostrap crashed.\n");
		return 1;
	}
    
    // Main loop
    int is_connected = 0;
    int first_connect = 1;
    while(1){
		is_connected = tox_isconnected(m);
		tox_do(m);
		// Do loop of first connect
		if(is_connected && first_connect){
			first_connect = 0;
			
			printf("Connected to DHT.\n");
			
			get_myid(m); // get my id
			
			int groupchat_num;
			groupchat_num = tox_add_groupchat(m);
			if(groupchat_num == -1){
				printf("Creating groupchat failed.\n");
				return 1;
			}
			if(groupchat_num == GROUPCHAT_NUMBER){
				printf("Setup groupchat [%i].\n",GROUPCHAT_NUMBER);
			}
			else{
				printf("Setup groupchat with undefined number.\n");
				return 1;
			}
			
			usleep(SLEEP_TIME_MAINLOOP);
		}
		// Do loops after first connect (save data and do short sleep)
		else if(is_connected && !first_connect){
			store_data(m);
			usleep(SLEEP_TIME_MAINLOOP);
		}
		// Catch error if connection is lost
		else if(!is_connected && !first_connect){
			printf("Lost connection to DHT.\n");
			return 1;
		}
		// Wait for connection
		else{
			usleep(SLEEP_TIME_ISNOTCONNECTED);
		}
	}
    
    // Kill tox
    tox_kill(m);
    
    return 0;
}
