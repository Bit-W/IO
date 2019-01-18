all: epoll
epoll:epoll.c  
	gcc $^ -o $@
select:select.c  
	gcc $^ -o $@
block:block.c  
	gcc $^ -o $@
test:test.c
	gcc $^ -o $@
