Project: Login User Supernode

Login: Login.c
	gcc -g -o Login Login.c -lsocket -lnsl

User: User.c
	gcc -g -o User User.c -lsocket -lnsl
	
Supernode: Supernode.c
	gcc -g -o Supernode Supernode.c -lsocket -lnsl

clean: 
	rm -rf Login
	rm -rf User
	rm -rf Supernode
