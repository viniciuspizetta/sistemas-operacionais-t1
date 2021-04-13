#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "bazar.h"
#include "include/arraylist.c"

#define NUM_THREADS_CLIENTE 10
#define NUM_THREADS_VOLUNTARIO 5

pthread_mutex_t mutex_doa;
pthread_mutex_t mutex_compra;
pthread_mutex_t mutex_move;

pthread_mutex_t mutex_aluga;
pthread_mutex_t mutex_entrega;

typedef struct argumentos
{
  arraylist *listaCompra;
  arraylist *listaReparo;
  arraylist *listaRoupasNovas;
  int nThread;

} argumentos;

typedef struct roupa_t
{
  unsigned int codigo;
  char *modelo;
  double preco;
  char *tamanho;

} roupa_t;

long random_at_most(long max)
{
  unsigned long

      num_bins = (unsigned long)max + 1,
      num_rand = (unsigned long)RAND_MAX + 1,
      bin_size = num_rand / num_bins,
      defect = num_rand % num_bins;

  long x;
  do
  {
    x = random();
  }

  // This is carefully written not to overflow
  while (num_rand - defect <= (unsigned long)x);

  // Truncated division is intentional
  return x / bin_size;
}

void *t_function_voluntario_remove_roupa(void *arg)
{
  argumentos *args;
  args = (argumentos *)arg;
  struct roupa_t *roupaComprar;
  pthread_mutex_lock(&mutex_compra);
  roupaComprar = (roupa_t *)arraylist_remove(args->listaCompra, 0);
  pthread_mutex_unlock(&mutex_compra);
  printf("Voluntario %i remove roupa mais antiga: %i \n", args->nThread, roupaComprar->codigo);
  return 0;
}

void *t_function_voluntario_doa_roupa(void *arg)
{
  argumentos *args;
  args = (argumentos *)arg;
  int index = (int)random_at_most(args->listaRoupasNovas->size - 1);

  if (args->listaCompra->size > 0)
  {
    struct roupa_t *doarRoupa;
    doarRoupa = (roupa_t *)arraylist_get(args->listaRoupasNovas, index);

    pthread_mutex_lock(&mutex_doa);
    arraylist_add(args->listaCompra, doarRoupa);
    arraylist_remove(args->listaRoupasNovas, index);
    pthread_mutex_unlock(&mutex_doa);
    printf("Voluntario %i doou uma roupa: %i \n", args->nThread, doarRoupa->codigo);
  }
  return 0;
}

void *t_function_voluntario_move_roupa(void *arg)
{
  argumentos *args;
  args = (argumentos *)arg;

  if (args->listaReparo->size > 0)
  {
    struct roupa_t *roupaEntregue;
    pthread_mutex_lock(&mutex_move);
    roupaEntregue = (roupa_t *)arraylist_pop(args->listaReparo);
    arraylist_add(args->listaCompra, roupaEntregue);
    pthread_mutex_unlock(&mutex_move);
    printf("Voluntario %i moveu uma roupa reparada: %i \n", args->nThread, roupaEntregue->codigo);
  }
  return 0;
}

void *t_function_cliente_pega_roupa(void *arg)
{
  while (1)
  {
    argumentos *args;
    args = (argumentos *)arg;

    int index = (int)random_at_most(args->listaCompra->size - 1); //-1 pois eh o tamanho e n a posicao do array
    struct roupa_t *roupaAlugado;
    roupaAlugado = (roupa_t *)arraylist_get(args->listaCompra, index);
    pthread_mutex_lock(&mutex_aluga);
    arraylist_remove(args->listaCompra, index); //remove do pool de novos
    //roupa* roupaAlugado = roupas_disponiveis_->pop_back();
    pthread_mutex_unlock(&mutex_aluga);
    printf("Cliente %i alugou o roupa: %i \n", args->nThread, roupaAlugado->codigo);
    sleep(5);
    pthread_mutex_lock(&mutex_entrega);
    arraylist_add(args->listaReparo, roupaAlugado);
    pthread_mutex_unlock(&mutex_entrega);
    //roupas_entregues_->push_front(roupaAlugado);
    printf("Cliente %i entregou o roupa: %i \n", args->nThread, roupaAlugado->codigo);
    sleep(5);
  }
}

void *t_function_voluntario_decideAcao(void *arg)
{
  long decisao;
  while (1)
  { //loop infinito
    decisao = random_at_most(2);

    switch (decisao)
    {
    case 0:
      t_function_voluntario_move_roupa(arg);
      break;
    case 1:
      t_function_voluntario_doa_roupa(arg);
      break;
    case 2:
      t_function_voluntario_remove_roupa(arg);
      break;
    default:
      break;
    }
    sleep(3);
  }
}

int main(int argc, char *argv[])
{

  // ArrayListroupa* roupas_disponiveis_ = new ArrayListRoupa();
  arraylist *roupas_disponiveis_;
  roupas_disponiveis_ = arraylist_create();

  //ArrayListRoupa* roupas_entregues_ = new ArrayListRoupa();
  arraylist *roupas_entregues_;
  roupas_entregues_ = arraylist_create();

  //ArrayListRoupa* roupas_novos_ = new ArrayListRoupa();
  arraylist *roupas_novos_;
  roupas_novos_ = arraylist_create();
  int i;
  for (i = 0; i < 100; i++)
  { //inicializa a lista de disponiveis com 20 roupas
    struct roupa_t *disp = malloc(sizeof(roupa_t));
    disp->codigo = i + 1; //nao pode ser random aqui porque deve ser UNIQUE
    char *tamanho;
    tamanho = (char *)'a'; //apenas um placeholder pro tamanho
    disp->tamanho = tamanho;
    disp->preco = (double)random_at_most(1000); //gera valor aleatorio pro roupa
    arraylist_add(roupas_disponiveis_, disp);
    //roupas_disponiveis_->push_back(disp); //joga roupa no array de roupas disp
  }

  for (i = 0; i < 100; i++)
  { //inicializa a lista de roupas novos a serem inseridos na lista de disponveis pelos voluntarios

    struct roupa_t *novo = malloc(sizeof(roupa_t));
    novo->codigo = i + 101; //nao pode ser random aqui porque deve ser UNIQUE, 21 pois eh o primeiro codigo apos os ja disponveis
    char *tamanho;
    tamanho = (char *)'a'; //apenas um placeholder pro tamanho
    novo->tamanho = tamanho;
    novo->preco = (double)random_at_most(1000); //gera valor aleatorio pro roupa entre 0 e 1000
    arraylist_add(roupas_novos_, novo);         //joga roupa no array de roupas novos
  }
  sleep(3);
  pthread_t tCli[NUM_THREADS_CLIENTE];    //array das threads cliente
  pthread_t tVol[NUM_THREADS_VOLUNTARIO]; //array das threads voluntario

  long *nomet1 = (long *)malloc(sizeof(long));
  *nomet1 = 1;

  argumentos *arrayArgs[10];

  for (i = 0; i < 10; i++)
  {
    struct argumentos *args = malloc(sizeof(argumentos));
    args->listaCompra = roupas_disponiveis_;
    args->listaReparo = roupas_entregues_;
    args->listaRoupasNovas = roupas_novos_;
    args->nThread = i + 1;
    arrayArgs[i] = args;
  }

  while (1)
  {
    for (i = 0; i < 5; i++)
    {
      printf("Voluntario %i se junta a bazar \n", i + 1);
      pthread_create(&tVol[i], NULL, t_function_voluntario_decideAcao, (void *)arrayArgs[i]);
    }
    for (i = 5; i < 10; i++)
    {
      printf("Cliente %i se torna cliente da bazar \n", i);
      pthread_create(&tCli[i], NULL, t_function_cliente_pega_roupa, (void *)arrayArgs[i]); // 1 eh numero da thread
    }
    sleep(9999);
  }
  return 0;
}