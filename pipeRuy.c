#include <stdio.h>
#include <errno.h>  															//fornecer� os relat�rios de erro do processo
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
 
char OUTPUT_FILE[] = "/tmp/compressao.gz";                                      //levar� o c�digo a arquivo tempor�rio compressao.gz

int main(){
	
  int pipe_fds[2];	
  int i;															            //pipe descritor, sendo pipe[1] a entrada e pipe[0] a saida
  
  if(pipe(pipe_fds) == -1){
    fprintf(stderr, "Falha ao criar pipe: %s \n", strerror(errno));          	//strerror converter� o c�digo do erro para uma linguagem textual
    return 1;																	//resultar em <0 ditar� o erro � chamada de pipe
  }	

  pid_t filho = fork();                                                         //process ID
  if(filho == -1){
    fprintf(stderr, "Falha ao criar filho: %s \n", strerror(errno));
    return 1;
  }

  if(filho == 0){
    if(dup2(pipe_fds[0], STDIN_FILENO) == -1){ 									//duplicar descritor  com stdin default em 0
      fprintf(stderr, "Falha ao associar pipe ao default: %s \n",
      strerror(errno));															
      _exit(1);																	//encerrar� o processo filho ao falhar
    }
    close(pipe_fds[0]);

    if(freopen(OUTPUT_FILE, "wb", stdout) == NULL){       						//write binary trocado ao acesso
      fprintf(stderr, "Falha ao criar sa�da: %s \n", strerror(errno));	
      _exit(1);
    }

    execlp("gzip", "gzip", "-9", NULL);											//a a��o duplicar� as a��es do shell na busca por um execut�vel com argv[] de refer�ncia
    fprintf(stderr, "Erro em gzip: %s\n", strerror(errno));
    _exit(1);
  }

  FILE *enviaAoGzip = fdopen(pipe_fds[1], "w");                                 //ponteiro respons�vel pela escrita
  if(enviaAoGzip == NULL){
    fprintf(stderr, "Falha ao associar stream: %s \n",
    strerror(errno));
    return 1;
  }

  struct sigaction exam;														//examinar� o sinal de exam com a struct sigaction
  memset(&exam, 0, sizeof exam);												//copiar� os dados para exam
  exam.exam_handler = SIG_IGN;												
  sigaction(SIGPIPE, &exam, NULL);												//ignora os sinais

  for(i = 0; i < 200; i++){														//200 linhas de arquivo
    errno = 0;
    fprintf(enviaAoGzip, "%d \n", i);
    if(errno != 0){
      fprintf(stderr, "Falha ao inserir "%d" no pipe: %s \n", i,
      strerror(errno));
      return 1;
    }
  }
  if(fclose(enviaAoGzip) == EOF){												//end of file
    fprintf(stderr, "Falha ao inserir no pipe: %s \n", strerror(errno));
    return 1;
  }

  int status;
  waitpid(filho, &status, 0);													//esperando pela mudan�a de status
  if(status != 0){
    fprintf(stderr, "Processo filho com erro (statu s= %d) \n", status);
    return 1;
  }

  return 0;
} 

//creditos de pesquisa: Paulo