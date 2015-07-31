#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 80

//Function that returns the address for a given name.
char* get_by_name(char *name,char o, char *logfile);
//Returns the address in char* format.
char* get_address(struct hostent *ent);
//Sets true for r,e,o and the value of logfile in case they are given.
int get_arguments(int no_of_arguments, char **arguments, bool *r, bool *e, bool *o, char **logfile);
//Sets the server_name and the path for GET.
int get_serverN_and_path(char *link,char **server_name,char **path,char o, char *logfile);
//Creates the website's primary folder.
void create_primary_folder (char* primaryFolder);
//Creates the folders recursively.
void create_folders(char *path);
//Downloads the data to a file.
int write_to_file(char * path, char * server_name, int sockfd,
		FILE **pFile, char **fileName, char * address, char o , char * logfile);
//Connects to the server.
void connect_to_server(int * sockfd,struct sockaddr_in toServer,char *address,char o, char*logfile);
//Gets all the links from a file.
int get_recursive_links(char *fileName, char** every_link,char e);
//Begins the recursive download if the option is selected.
void recursive_download(char ** links, int size ,int level, char *address ,
		char *server_name,char * directory_path, char o , char * logfile, char e);
//Gets the path for GET in the recursive function.
void get_path(char *link , char ** path, char *orig);

int main(int argc, char **argv) {
	//Not enough or too many arguments.
	if ((argc < 2) || (argc > 6)) {
		char err[1024];
		sprintf(err,"%s","[USAGE] ./my_wget [-r] [-e] [-o <log file>] http://<server_name>/<path> !\n");
    	fputs(err,stderr);
    	return 0;
    }
	//booleans for -r -e -o options
	bool r = false;
    bool e = false;
    bool o = false;

    //logfile for -o option
    char *logfile;

    int retval = get_arguments(argc,argv,&r,&e,&o,&logfile);
    if (retval != 0){
    	return retval;
    }

    //the full link.
    char link[1024];
    memset(link,0,sizeof(link));
    strcpy(link,argv[argc-1]);

    //cuts the link in two parts.
    char *server_name;
    char *path = new char[10240];
    memset(path,0,sizeof(path));
    retval = get_serverN_and_path(link,&server_name,&path,o,logfile);
    if (retval != 0){
    	return retval;
    }
    create_primary_folder(server_name);

   //Gets only the path for mkdir
    std::string stringToken(path);
    char directory_path[10240];
    strcpy(directory_path,stringToken.substr(1,stringToken.find_last_of("/")).c_str());
    create_folders(directory_path);
    if (retval != 0){
    	return retval;
    }

    char *address = get_by_name(server_name,o,logfile);
    if (strcmp(address,"error") == 0){
    	return 11;
    }

    struct sockaddr_in toServer;
    int sockfd;

    connect_to_server(&sockfd,toServer,address,o,logfile);

    //Writes the first file.
    FILE *pFile;
    char * fileName;
	write_to_file(path,server_name,sockfd,&pFile,&fileName,address,o,logfile);
	close(sockfd);
	fclose(pFile);

	//if recursion is chosen , we call the recursive function
	//recursive_download which has every link from the written
	//link, their number, level of recursion,address,server name,
	//the directory, the -o option with it's logfile and -e option.
	if ( r == true ){
		int size;
		char ** links;
		links = new char*[1024];
		for (int i = 0 ; i < 1024; i++){
			links[i] = new char[1024];
		}
		size = get_recursive_links(fileName,links,e);
		recursive_download(links,size,2,address,server_name,directory_path,o,logfile,e);
		for (int i = 0 ; i < 1024; i++){
			delete [] links[i];
		}
		delete [] links;
	}

	delete[] path;

	return 0;
}

char* get_by_name(char *name,char o,char *logfile) {
    struct hostent *address = gethostbyname(name);
    if (address == NULL) {
        char err[1024];
        sprintf(err,"%s%s%s","[ERROR] Cannot get address for name: ",name,"\n");
        //if -o was chosen , writes in the logfile the error
        if (o==true){
        	struct stat buffer;
        	FILE *pFile;
        	if ( stat( logfile, &buffer ) == -1){
        		pFile = fopen(logfile,"w+");
        	}
        	else {
        		pFile = fopen(logfile,"a");
        	}
        	fwrite(err,strlen(err),sizeof(char),pFile);
        	fclose(pFile);
        }
        //else it writes it to the stderr
        else {
        	fputs(err,stderr);
        }
        char * error;
        memcpy(error,"error",strlen(error));
        return error;
    }
    return get_address(address);
}

char* get_address(struct hostent *address) {
    int i;
    char *p;
    i = 0;
    struct in_addr** addr_list = (struct in_addr**) address->h_addr_list;
    while (addr_list[i] != NULL) {
	   p = inet_ntoa(*addr_list[i++]);
    }
    return p;
}

int get_arguments(int no_of_arguments, char **arguments, bool *r, bool *e, bool *o, char **logfile){
	//if we have at least 3 arguments,
	if (no_of_arguments > 2){
	   	char* argument;
	   	//iterate through them and compare them with -r -e and -o.
	   	for (int i = 1 ; i < no_of_arguments-1 ; i++){
	   		if (strstr(arguments[i],"-r")){
	   			*r = true;
	   		}
	   		else if (strstr(arguments[i],"-e")){
	   			*e = true;
	   		}
	   		else if (strstr(arguments[i],"-o")){
	   			//if the next one is the last argument, it is the link.
	   			//if r|e is false and -r|-e is next , it means that they've been given in a wrong order
	  			if ((i+1 == no_of_arguments-1)||
	  				((*r == false)&&(strstr(arguments[i+1],"-r") != NULL)&&(strlen(arguments[i+1])==2))
	  				||((*e == false)&&(strstr(arguments[i+1],"e") != NULL)&&(strlen(arguments[i+1])==2))){
	  				char err[1024];
	  				sprintf(err,"%s","[ERROR: Missing log file or wrong oder] "
	  						"[USAGE]: ./my_wget [-r] [-e] [-o <log file>]\n");
	  				fputs(err,stderr);
	    			return 2;
	  			}
	  			//othewhise the -o argument is valid.
	    		else {
	    			*o = true;
	    			*logfile = arguments[i+1];
	    			i++;
	    		}
	    	}
	   		//if neither of them exists as an argument , it's an error.
	    	else {
	    		char err[1024];
	    		sprintf(err,"%s%s%s","[ERROR]: Wrong argument: ", arguments[i],"\n");
	    		fputs(err,stderr);
	    		return 3;
	    	}
	    }
	}
	return 0;
}
int get_serverN_and_path(char *link,char **server_name,char **path,char o , char *logfile){
	//if there is no "htm" or "html" , it means that the link is invalid.
	if (strstr(link,"htm") == NULL){
		char err[1024];
		sprintf(err,"%s%s%s","[ERROR]: Invalid link(missing html/htm): ",link,"\n");
		if ( o == true ){
			char newFileName[1024];
			struct stat buffer;
			FILE *pFile;
			if ( stat( logfile, &buffer ) == -1){
				pFile = fopen(logfile,"w+");
			}
			else {
				pFile = fopen(logfile,"a");
			}
			fwrite(err,strlen(err),sizeof(char),pFile);
		}
		else {
			fputs(err,stderr);
		}
		return 4;
	}
	//otherwhise we separate the server name from the rest of the link.
	//(The function change the value of server_name and path by their address.
	char *token = strtok(link,"/");
	token = strtok(NULL,"/");
	*server_name = token;

	token = strtok(NULL,"\0");
    strcpy(*path,"/");
    strcat(*path,token);
    return 0;
}
void create_primary_folder (char* primaryFolder){
	char aux[10240];
	strcpy(aux,primaryFolder);
	strcat(aux,"/");
	mkdir(aux,0777);
	//we change the current directory to the primary folder(server name)
	chdir(aux);
}
void create_folders(char *path){
	char * mkdirPath = new char[10240];
	sprintf(mkdirPath, "%s%s", "mkdir -p ", path);
	//the -p option makes them recursively
	if (strcmp(mkdirPath,"mkdir -p ") != 0){
		system(mkdirPath);
	}
	delete[] mkdirPath;
}

int write_to_file(char * path, char * server_name,int sockfd,FILE **pFile,
		char **fileName,char * address,char o, char * logfile){

	char buffer[10240];
	char newFileName[1024];
	//we need to go back to the home folder (we are currently in the server folder)
	if (o == true){
		strcpy(newFileName,"../");
		strcat(newFileName,logfile);
	}
	memset(buffer,0,sizeof(buffer));
	// "GET" <path>"\n" makes it so it has no header.
	sprintf(buffer, "%s%s%s%s%s", "GET ", path, " \nHTTP/1.0\nHost: " ,server_name  ,"\n\n");
	//if path is "/" (invalid) => error
	if (strlen(path) == 1){
		if (o == true){
			char err[1024];
			struct stat buffer;
			if ( stat( newFileName, &buffer ) == -1){
				*pFile = fopen(newFileName,"w+");
			}
			else {
				*pFile = fopen(newFileName,"a");
			}
			sprintf(err,"%s%s%s","[ERROR] Invalid path: ",path,"\n");
			fwrite(err,strlen(err),sizeof(char),*pFile);
			fclose(*pFile);
			return 4;
		}
		else{
			char err[1024];
			sprintf(err,"%s%s%s","[ERROR] Invalid path: ",path,"\n");
			fputs(err,stderr);
			return 4;
		}
	};
	char buffer2[10240];
	sprintf(buffer2, "%s%s%s%s%s", "HEAD ", path, " HTTP/1.0\nHost: " ,server_name  ,"\n\n");

	//send command for header
	send(sockfd, buffer2, strlen(buffer2), 0);
	memset(buffer2,0,sizeof(buffer2));
	//receive the header.
	int size = recv(sockfd,buffer2,sizeof(buffer2),0);
	close(sockfd);

	char head[10240];
	memset(head,0,sizeof(head));
	int ok = true;
	//read every byte from the header
	for(int i = 0; i < strlen(buffer2); i++) {
		//if we find '4' => error => not ok
		if (buffer2[i] == '4'){
			ok = false;
		}
		//if we find "Data" sequence , it means that we got the first line.
		if ((buffer2[i] == 'D')&&(buffer2[i+1] == 'a')&&(buffer2[i+2] == 't')&&(buffer2[i+3] == 'e')){
			break;
		}
		//take the first line of the header in "head" .
		head[i] = buffer2[i];
	}
	//if it's not ok , write the error to stderr or logfile
	if (!ok){
		if (o == true){
			//if file exists, append to it,
			//if it doesn't , create it.
			struct stat buffer;
			if ( stat( newFileName, &buffer ) == -1){
				*pFile = fopen(newFileName,"w+");
			}
			else {
				*pFile = fopen(newFileName,"a");
			}
			fwrite(head,strlen(head),sizeof(char),*pFile);
			fclose(*pFile);
		}
		else {
			fputs(head,stderr);
		}
	}
	struct sockaddr_in toServer;
	connect_to_server(&sockfd,toServer,address,o,logfile);

	*fileName = path + 1;
	*pFile = fopen(*fileName,"w+");
	send(sockfd, buffer, strlen(buffer), 0);
	int rsize=0;
	//write the data from server to file.
	do{
		memset(buffer,0,sizeof(buffer));
		rsize = recv(sockfd, buffer, sizeof(buffer), 0);
		fwrite(buffer,rsize,sizeof(char),*pFile);
	}while (rsize != 0);

	return 0;
}

void connect_to_server(int * sockfd,struct sockaddr_in toServer,char *address,char o,char *logfile){
	*sockfd = socket(PF_INET, SOCK_STREAM, 0);

	toServer.sin_family = AF_INET;
	toServer.sin_port = htons(PORT);

	inet_aton(address, &toServer.sin_addr);

	int connection = connect(*sockfd, ((struct sockaddr*) &toServer), sizeof(struct sockaddr));
	if (connection != 0){
		char err[1024];
		sprintf(err,"%s","[ERROR] Cannot connect to server!\n");
		if (o == true){
			FILE *pFile;
			struct stat buffer;
			if ( stat( logfile, &buffer ) == -1){
				pFile = fopen(logfile,"w+");
			}
			else {
				pFile = fopen(logfile,"a");
			}
			fwrite(err,strlen(err),sizeof(char),pFile);
			fclose(pFile);
		}
		fputs(err,stderr);
	}
}

int get_recursive_links(char *fileName, char** every_link,char e){
	FILE * pFile = fopen(fileName,"rb");
	std::string all_links;
	char c;
	int counter = 0;//for <a
	int counter2 = 0;//for href=" or href='
	//for removing 1 quote("/this.html""example.html")
	int quoteCounter = 0; //from the string
	int i = 0;
	while((c = fgetc(pFile)) != EOF) {
		if (c=='<'){
			if (fgetc(pFile) == 'a'){
				counter = 1;
			}
		}
		if ((counter == 1)&&(c != '>')){
			switch (counter2){
			case 0: c=='h'?counter2++:counter2=0;
					break;
			case 1: c=='r'?counter2++:counter2=0;
					break;
			case 2: c=='e'?counter2++:counter2=0;
					break;
			case 3: c=='f'?counter2++:counter2=0;
					break;
			case 4: c=='='?counter2++:counter2=0;
					break;
			case 5: ((c=='\"')||(c=='\''))?counter2++:counter2=0;
					break;
			}
		}
		//if we have href=" or href='
		if ((counter2 == 6)&&(c != '>')){
			//this basically replaces a " or ' character with space.
			if (((c=='\"') || (c=='\''))&& ((quoteCounter % 2) != 0) ){
				quoteCounter ++;
				all_links += " ";
			}
			else if (((c=='\"') || (c=='\'')) && ((quoteCounter % 2) == 0) ){
				quoteCounter++;
			}
			else{
				all_links += c;
			}
		}
		//resets counters if we find ">" and we had "<a"
		else if ((c == '>')&&(counter==1)) {
			counter = 0;
			counter2 = 0;
		}
	}
	char * char_link = new char[10240];
	strcpy(char_link,all_links.c_str());
	char *token = strtok(char_link," ");
	int size = 0;
	while (token != NULL){
		//if we have the -e option we take every .*** or .**** extensions.
		if (e == true){
			if ((strstr(token,"http") == NULL) && (strchr(token,'.'))){
				std::string tokstring(token);
				int position = tokstring.find_last_of(".");
				tokstring = tokstring.substr(position+1,strlen(tokstring.c_str()));
				if (((strlen(tokstring.c_str())==3)||(strlen(tokstring.c_str())== 4))
						&&(tokstring.find_last_of("/") == std::string::npos)){
					strcpy(every_link[size],token);
					size++;
				}
			}
		}
		//else only htm and html.
		else{
			if ((strstr(token,"htm") != NULL)&&(strstr(token,"http") == NULL) && (strstr(token,"#")) == NULL){
				strcpy(every_link[size],token);
				size++;
			}
		}
		token = strtok(NULL," ");
	}
	delete[] char_link;
	fclose(pFile);
	return size;
}

void recursive_download(char ** links, int size, int level, char *address,
		char *server_name,char * directory_path, char o , char *logfile, char e){

	//if we are on level 6 , return (we've been through 5 levels of recursion)
	if (level == 6){
		return;
	}
	else{
		char ** fileNames = new char *[size];
		char ** path = new char *[size];
		for (int i = 0 ; i < size; i++){
			fileNames[i] = new char[1024];
			memset(fileNames[i],0,1024);
		}
		for (int i = 0 ; i < size ; i++){
			path[i] = new char[1024];
			memset(path[i],0,1024);
		}
		for (int i = 0 ; i < size ; i++){
			std::string stringToken(links[i]);
			//create the directory path
			if (links[i][0] != '/'){
				strcat(path[i],directory_path);
			}
			int lastof = stringToken.find_last_of("/");
			//if we have a "/" , we add to the path
			//everything until that and a "/" after.
			if (lastof>0){
				strcat(path[i],stringToken.substr(0,lastof).c_str());
				strcat(path[i],"/");
			}
			//if the first the link begins with / we skip it.
			if(links[i][0] == '/'){
				strcpy(path[i],path[i]+1);
			}
			//create the structure of folders.
			create_folders(path[i]);
			char *download_path = new char [10240];
			//we get the path for the recursive function.
			get_path(links[i],&download_path,path[i]);
			fileNames[i] = strcpy(fileNames[i],download_path+1);
			char * fileName;
			//if file doesn't exist we download it
			struct stat buffer;
			if ( stat( download_path+1, &buffer ) == -1){
				struct sockaddr_in toServer;
				int sockfd;
				connect_to_server(&sockfd,toServer,address,o,logfile);
				FILE *pFile;
				char *sn;
				write_to_file(download_path,server_name,sockfd,&pFile,&fileName,address,o,logfile);
				fclose(pFile);
				close(sockfd);

			}
			//else we don't.
			else {
				continue;
			}
			delete[] download_path;
		}
		//get the neighbors for every downloaded file and call the function on the next level.
		for (int i = 0 ; i < size ; i ++){
			int Size;
			char ** mylinks;
			mylinks = new char*[1024];
			for (int j = 0 ; j < 1024; j++){
				mylinks[j] = new char[1024];
				memset(mylinks[j],0,1024);
			}
			Size = get_recursive_links(fileNames[i],mylinks,e);
			recursive_download(mylinks,Size,level+1,address,server_name,path[i],o,logfile,e);
			for (int j = 0 ; j < 1024; j++){
				delete [] mylinks[j];
			}
			delete [] mylinks;
		}
		for (int j = 0 ; j < size; j++){
			delete [] fileNames[j];
		}
		delete [] fileNames;
		for (int j = 0 ; j < size ; j ++){
			delete[] path[j];
		}
	}
}
void get_path(char *link , char ** path, char * origin){
	std::string mystring(link);
	int gasit = mystring.find_last_of("/");
	if (gasit > 0){
		mystring = mystring.substr(gasit,strlen(mystring.c_str())).c_str();
	}
	strcpy(*path,"/");
	strcat(*path,origin);
	if(mystring.c_str()[0] == '/'){
		strcat(*path,mystring.c_str()+1);
	}
	else {
		strcat(*path,mystring.c_str());
	}
}
