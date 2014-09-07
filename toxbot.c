#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tox/tox.h>

/* SETUP */

#define SLEEP_TIME_ISNOTCONNECTED 100000
#define SLEEP_TIME_MAINLOOP 10000
#define BOOTSTRAP_ADDRESS "192.254.75.98"
#define BOOTSTRAP_PORT 33445
#define BOOTSTRAP_KEY "951C88B7E75C867418ACDB5D273821372BB5BD652740BCDF623A4FA293E75D2F"

#define MY_NAME "Toxbot"
#define STATUS_MSG "Write 'help' if you dont know what to do!"
#define SAVEFILE "savetox.bin"

#define MSG_INVITE "invite"
#define RETURN_MSG_INVITE "I have invited you to a groupchat!"
#define RETURN_MSG_NOTINVITE "Dude, dunno what you want..."
#define GROUPCHAT_NUMBER 0
#define SAVEFILE_MSG "savemsg.txt"

#define MSG_LOG "log"
#define MSG_LOG_DEFAULT_LINES 2
#define MSG_LOG_MAX_LINES 1000
#define MSG_LOG_DEFAULT_LINE_LENGTH 256

#define MSG_HELP "help"
#define RETURN_MSG_HELP "Valid commands:\ninvite\tinvites to groupchat\nlog\tsends log\nlog n\tsends log with length n\nhelp\tgives help\n"

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

/* CALLBACK AND FUNCTIONS: ON REQUEST */

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

/* CALLBACK AND FUNCTIONS: ON MESSAGE */

void
send_log(Tox *m, int32_t friendnumber, int32_t lines){
	printf("Send log to [%i] with %i lines.\n",friendnumber,lines);
	FILE *fd = fopen(SAVEFILE_MSG, "r");
	if(fd!=NULL){; // jump out if there is no file
		fseek(fd, 0, SEEK_END);
		int len = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		
		// Check if enough data is available for lines msg parts, else pointer remains on first char
		if(len-lines*MSG_LOG_DEFAULT_LINE_LENGTH>=0){
			fseek(fd, len-lines*MSG_LOG_DEFAULT_LINE_LENGTH, SEEK_SET);
			len = lines*MSG_LOG_DEFAULT_LINE_LENGTH;
		}

		char *buf = malloc(len);
		fread(buf, len, 1, fd);
		
		// Send log to friend
		if(len>TOX_MAX_MESSAGE_LENGTH){ // split msg if it is too long
			char buf_split[TOX_MAX_MESSAGE_LENGTH];
			int len_split, i;
			// Send full packages
			for(i=0;i<len/(int)TOX_MAX_MESSAGE_LENGTH;i++){
				memcpy(buf_split,buf+i*TOX_MAX_MESSAGE_LENGTH,TOX_MAX_MESSAGE_LENGTH*sizeof(uint8_t));
				len_split = TOX_MAX_MESSAGE_LENGTH;
				tox_send_message(m,friendnumber,buf_split,len_split);
			}
			// Send last package
			memcpy(buf_split,buf+(len/(int)TOX_MAX_MESSAGE_LENGTH)*TOX_MAX_MESSAGE_LENGTH,(len-(len/(int)TOX_MAX_MESSAGE_LENGTH)*TOX_MAX_MESSAGE_LENGTH)*sizeof(uint8_t));
			len_split = len-(len/(int)TOX_MAX_MESSAGE_LENGTH)*TOX_MAX_MESSAGE_LENGTH;
			if(len>0) tox_send_message(m,friendnumber,buf_split,len_split);
		}
		else{
			tox_send_message(m,friendnumber,buf,len);
		}
		
		free(buf);
	}
}

void
on_message(Tox *m, int32_t friendnumber, const uint8_t *string, uint16_t length, void *userdata){
	printf("Msg [%i]: %s\n",friendnumber,string);
	uint8_t *msg_invite, *msg_log, *msg_help, *m_msg_log;
	msg_invite = MSG_INVITE;
	msg_log = MSG_LOG;
	msg_help = MSG_HELP;
	m_msg_log = malloc(strlen(MSG_LOG)*sizeof(uint8_t)); // copy only chars of MSG_LOG to string for checking of numbers behind MSG_LOG
	memcpy(m_msg_log,string,strlen(MSG_LOG)*sizeof(uint8_t));
	
	// Take aktion based on string
	if(!memcmp(msg_invite,string,length*sizeof(uint8_t))){ // if msg is MSG_INVITE
		send_log(m, friendnumber,MSG_LOG_DEFAULT_LINES);
		tox_invite_friend(m,friendnumber,GROUPCHAT_NUMBER);
		tox_send_message(m,friendnumber,RETURN_MSG_INVITE,strlen(RETURN_MSG_INVITE));
		printf("Invited [%i] to groupchat.\n",friendnumber);
	}
	else if(!memcmp(msg_log,m_msg_log,strlen(MSG_LOG)*sizeof(uint8_t))){ //if msg is MSG_LOG
		int32_t lines;
		if(length-strlen(MSG_LOG)>1){ // if string is long enough that there can be a number
			uint8_t *lines_string = malloc(length-strlen(MSG_LOG)-1); // take care of whitespace!
			memcpy(lines_string,string+strlen(MSG_LOG)+1,(length-strlen(MSG_LOG)-1)*sizeof(uint8_t));
			lines = atoi(lines_string);
			if(lines>MSG_LOG_MAX_LINES) lines = MSG_LOG_DEFAULT_LINES; // fallback
		}
		else{ // if string is only len(MSG_LOG) length long, set number to default
			lines = MSG_LOG_DEFAULT_LINES;
		}
		send_log(m, friendnumber, lines); // send log with lines lines to friend
	}
	else if(!memcmp(msg_help,string,length*sizeof(uint8_t))){
		tox_send_message(m,friendnumber,RETURN_MSG_HELP,strlen(RETURN_MSG_HELP));
	}
	else{ // fallback
		tox_send_message(m,friendnumber,RETURN_MSG_NOTINVITE,strlen(RETURN_MSG_NOTINVITE));
		printf("Unknown command from [%i].\n",friendnumber);
	}
	
	free(m_msg_log);
}

/* CALLBACK AND FUNCTIONS: ON CONNECTION STATUS CHANGE */

void
on_connection_status(Tox *m, int32_t friendnumber, uint8_t status, void *userdata){
	if(status){
		printf("Friend %i gone offline.\n",friendnumber);
	}
	else{
		printf("Friend %i comes online.\n",friendnumber);
	}
}

/* CALLBACK AND FUNCTIONS: ON GROUP MESSAGE */

void
store_group_message(Tox *m, uint8_t *name, uint16_t name_len, const uint8_t *msg, uint16_t msg_len){
	// Get time
	time_t rawtime;
	struct tm * timeinfo;
	uint8_t buffer[100];
	uint8_t time_string[100];
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime(buffer,100,"%R %a %b %d",timeinfo);
	int time_len = sprintf(time_string,"[%s]",buffer);
	
	// Set string
	int len = name_len+1+time_len+1+msg_len+2;
	uint8_t *buf = malloc(len*sizeof(uint8_t));
	
	memcpy(buf,name,name_len*sizeof(uint8_t)); // copy name
	memset(buf+name_len,' ',sizeof(uint8_t)); // set whitespace
	memcpy(buf+name_len+1,time_string,time_len*sizeof(uint8_t)); // copy time
	memset(buf+name_len+1+time_len,'\n',sizeof(uint8_t)); // set newline
	memcpy(buf+name_len+1+time_len+1,msg,msg_len*sizeof(uint8_t)); // copy msg
	memset(buf+name_len+1+time_len+1+msg_len,'\n',sizeof(uint8_t)); // set newline
	memset(buf+name_len+1+time_len+1+msg_len+1,'\n',sizeof(uint8_t)); // set newline
	
	// Save string
	FILE *fd = fopen(SAVEFILE_MSG, "a");
    fwrite(buf, len, 1, fd);
    free(buf);
    fclose(fd);
}

void
on_group_message(Tox *m, int groupnum, int peernum, const uint8_t *msg, uint16_t msg_len, void* userdata){	
	uint8_t buf_name[TOX_MAX_NAME_LENGTH];
	uint8_t *name;
	uint16_t name_len;
	name_len = tox_group_peername(m, groupnum, peernum, buf_name);
	if(name_len!=-1){
		name = malloc(name_len*sizeof(uint8_t));
		memcpy(name,buf_name,name_len*sizeof(uint8_t));
		printf("Group msg [%s]: %s\n", name, msg);
		// Store msg
		store_group_message(m, name, name_len, msg, msg_len);
	}
	else printf("Cant resolve peername!\n");
}

/* INIT TOX */

static Tox*
init_tox(void)
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
    tox_callback_group_message(m, on_group_message, NULL);
    
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
