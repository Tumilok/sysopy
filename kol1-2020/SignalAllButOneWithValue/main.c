#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>


void sighandler(int signo, siginfo_t *siginfo, void *cont) {
    printf("Child: %d\n", siginfo -> si_value.sival_int);
}


int main(int argc, char* argv[]) {

    if(argc != 3){
        printf("Not a suitable number of program parameters\n");
        return 1;
    }

    struct sigaction action;
    action.sa_sigaction = &sighandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);

    //..........


    int child = fork();
    if(child == 0) {
        //zablokuj wszystkie sygnaly za wyjatkiem SIGUSR1
        //zdefiniuj obsluge SIGUSR1 w taki sposob zeby proces potomny wydrukowal
        //na konsole przekazana przez rodzica wraz z sygnalem SIGUSR1 wartosc
        sigprocmask(SIG_BLOCK, &mask, NULL);
        sigaction(SIGUSR1, &action, NULL);
        pause();
    }
    else {
        //wyslij do procesu potomnego sygnal przekazany jako argv[2]
        //wraz z wartoscia przekazana jako argv[1]
        sleep(1);
        union sigval value;
        value.sival_int = atoi(argv[1]);
        sigqueue(child, atoi(argv[2]), value);
    }

    return 0;
}
