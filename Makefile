target = echosrv_nb_main echocli_nb_main echosrv_readline_main echosrv_select_main echosrv_epoll_main echocli_main echocli_select_main echosrv_msq_main echocli_msq_main echosrv_pthread_main produce_consume produce_consume_cond

all:$(target)

echosrv_nb_main: echosrv_nb.o echosrv_nb_main.o
	g++ $^ -o $@
echocli_nb_main: echocli_nb.o echocli_nb_main.o
	g++ $^ -o $@

echosrv_readline_main: echosrv_readline.o echosrv_readline_main.o
	g++ $^ -o $@

echosrv_select_main: echosrv_select.o echosrv_select_main.o
	g++ $^ -o $@

echocli_main: echocli.o echocli_main.o
	g++ $^ -o $@

echocli_select_main: echocli_select.o echocli_select_main.o
	g++ $^ -o $@

#echosrv_poll_main: echosrv_poll.o echosrv_poll_main.o
#	g++ $^ -o $@

echosrv_epoll_main: echosrv_epoll.o echosrv_epoll_main.o
	g++ $^ -o $@

echosrv_msq_main: echosrv_msq.o echosrv_msq_main.o
	g++ $^ -o $@

echocli_msq_main: echocli_msq.o echocli_msq_main.o
	g++ $^ -o $@

echosrv_pthread_main: echosrv_pthread.o echosrv_pthread_main.o
	g++ $^ -o $@ -lpthread

produce_consume:
	g++ -o $@ produce_consume.cpp -pthread

produce_consume_cond:
	g++ -o $@ produce_consume_cond.cpp -pthread

%.o: %.cpp myecho.h
	g++ -c $^

.PHONY:clean
clean:
	rm -rf *.o $(target)
