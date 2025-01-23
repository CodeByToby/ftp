all: client server

client:
	@echo "Building the client..."
	make -C Client

server:
	@echo "Building the server..."
	make -C Server

clean:
	@echo "Cleaning both client and server..."
	make -C Client clean
	make -C Server clean