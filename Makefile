SERVER_OUT := Server/ftp_server
CLIENT_OUT := Client/ftp_client

all: $(SERVER_OUT) $(CLIENT_OUT)

$(CLIENT_OUT):
	@echo -e "\n\tBuilding the client...\n"
	make -C Client

$(SERVER_OUT):
	@echo -e "\n\tBuilding the server...\n"
	make -C Server

clean:
	@echo -e "\n\tCleaning both client and server...\n"
	make -C Client clean
	make -C Server clean