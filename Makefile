SERVER_OUT := Server/build/server
CLIENT_OUT := Client/build/client

all: $(SERVER_OUT) $(CLIENT_OUT)
server: $(SERVER_OUT)
client: $(CLIENT_OUT)

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

.PHONY: all server client clean