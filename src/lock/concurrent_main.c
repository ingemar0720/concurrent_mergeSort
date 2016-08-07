#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<sys/time.h>
#include<stdbool.h>
#include<pthread.h>
#include <unistd.h>

#define size 100000
#define range 20000
#define max_thread_num 8
#define divided_remalloc_base 64
#define dump_data

int duration;
pthread_mutex_t gmutex;
pthread_cond_t cv;
pthread_t *threads;
pthread_attr_t attr;
int thread_count;
int divided_count;

/* Link list node */
typedef struct node
{
	int data;
	struct node* next;
	bool divided;//set flag to 1 if already divided
}node_t;

node_t **divided_table;//pointer array, each array element store pointer of divided list with acsending sequence.

/* function prototypes */
node_t *SortedMerge(node_t *a, node_t *b);
void FrontBackSplit(node_t *source, node_t **frontRef, node_t **backRef);
void printList(node_t *node);

/* merge all element in divided_table */
void merge_table(void){
	int index;
	bool is_odd = false;
	int tmp_divided_count = divided_count;

	tmp_divided_count--;
	while(tmp_divided_count>1){
		if(tmp_divided_count%2 == 1){
			is_odd = true;
		}else{
			is_odd = false;
		}
		if(is_odd)
			tmp_divided_count--;
		for(index=0; index<tmp_divided_count; index+=2){
			divided_table[index>>1] = SortedMerge(divided_table[index], divided_table[index+1]);
		}

		if(is_odd){
			divided_table[(index>>1)] = divided_table[tmp_divided_count];
			tmp_divided_count=(index>>1)+1;
		}else{
			tmp_divided_count = index>>1;
		}
	}

	return ;
}

/* Function to insert a divided list to divided_table */
void push_table(node_t *new_node)
{
	int base_count = divided_count & 0x3f;
	pthread_mutex_lock(&gmutex);
	/* allocate node */
	if(base_count == 0){
		divided_table = realloc(divided_table, (divided_count>>6) * divided_remalloc_base*sizeof(node_t*));
	}
	divided_table[divided_count-1] = new_node;
	divided_count++;
	pthread_mutex_unlock(&gmutex);
}

/* single thread mergeSort */
void ST_MergeSort(node_t **headRef)
{
  struct node *head = *headRef;
  struct node *a;
  struct node *b;

  /* Base case -- length 0 or 1 */
  if ((head == NULL) || (head->next == NULL))
  {
  	head->divided = true;
    return;
  }

  /* Split head into 'a' and 'b' sublists */
  FrontBackSplit(head, &a, &b);

  /* Recursively sort the sublists */
  ST_MergeSort(&a);
  ST_MergeSort(&b);

  /* answer = merge the two sorted lists together */
  *headRef = SortedMerge(a, b);
}

/*bool isDivided(node_t *data){
	if(data->next == NULL)
		return true;
	while(data != NULL){
		if(data->divided == false)
			return false;
		data = data->next;
	}
	return true;
}*/

int getMemberLevel(node_t *data){
	int level = 0;
	while (data!=NULL){
		level++;
		data = data->next;
	}
	return level;
}

/* sorts the linked list by changing next pointers (not data) */
void *MT_MergeSort(void *par)
{
	node_t **head_tmp = (node_t**)par;
	node_t *head = *head_tmp;
	node_t *a;
	node_t *b;
	int thread_a_num = 0;
	int thread_b_num = 0;
	int ret = 0;
	bool a_isDivided = false;
	bool b_isDivided = false;

	/* Base case -- length 0 or 1 */
	if ((head == NULL) || (head->next == NULL))
	{
		return NULL;
	}

	/* Split head into 'a' and 'b' sublists */
	FrontBackSplit(head, &a, &b); 
	
	/* the splitted list should contain at least 4 members */
	if(getMemberLevel(a)>3){
		if(thread_count < max_thread_num){
			thread_a_num = thread_count;
			if(pthread_create(&threads[thread_count++], &attr, MT_MergeSort, (void*)(&a)) != 0){
				fprintf(stderr, "create thread fail\n");
				exit(1);
			}
			a_isDivided = true;
		}else{
			ST_MergeSort(&a);
		}
	}else{
		ST_MergeSort(&a);
	}

	/* the splitted list should contain at least 4 members */
	if(getMemberLevel(b)>3){
		if(thread_count < max_thread_num){
			thread_b_num = thread_count;
			if(pthread_create(&threads[thread_count++], &attr, MT_MergeSort, (void*)(&b)) != 0){
				fprintf(stderr, "create thread fail\n");
				exit(1);
			}
			b_isDivided = true;
		}else{
			ST_MergeSort(&b);
		}
	}else{
		ST_MergeSort(&b);
	}


	if(a_isDivided == true && b_isDivided == true){
	}else{
		if(a_isDivided == true){
			head = SortedMerge(NULL,b);
		}else if(b_isDivided == true){
			head = SortedMerge(a,NULL);
		}else{
			head = SortedMerge(a,b);
		}
		push_table(head);
	}
	
	/* wait until divided thread finish */
	if(thread_a_num > 0){
		pthread_join(threads[thread_a_num], NULL);
	}
	if(thread_b_num > 0){
		pthread_join(threads[thread_b_num], NULL);
	}
	pthread_exit(&ret);
	return NULL;
}

node_t *SortedMerge(node_t *a, node_t *b)
{
	node_t *result = NULL;

	/* Base cases */
	if (a == NULL)
		return(b);
	else if (b==NULL)
		return(a);

	/* Pick either a or b, and recur */
	if (a->data <= b->data)
	{
		result = a;
		result->next = SortedMerge(a->next, b);
	}
	else
	{
		result = b;
		result->next = SortedMerge(a, b->next);
	}
	return(result);
}

/* UTILITY FUNCTIONS */
/* Split the nodes of the given list into front and back halves,
   and return the two lists using the reference parameters.
   If the length is odd, the extra node should go in the front list.
   Uses the fast/slow pointer strategy.  */
void FrontBackSplit(node_t *source,
		node_t **frontRef, node_t **backRef)
{
	node_t *fast;
	node_t *slow;
	if (source==NULL || source->next==NULL)
	{
		/* length < 2 cases */
		*frontRef = source;
		*backRef = NULL;
	}
	else
	{
		slow = source;
		fast = source->next;

		/* Advance 'fast' two nodes, and advance 'slow' one node */
		while (fast != NULL)
		{
			fast = fast->next;
			if (fast != NULL)
			{
				slow = slow->next;
				fast = fast->next;
			}
		}

		/* 'slow' is before the midpoint in the list, so split it in two
		   at that point. */
		*frontRef = source;
		*backRef = slow->next;
		slow->next = NULL;
	}
}

/* Function to print nodes in a given linked list */
void printList(node_t *node)
{
	while(node!=NULL)
	{
		printf("%d\n ", node->data);
		node = node->next;
	}
}

/* Function to print nodes in a given linked list */
void freeList(node_t *node)
{
	node_t *tmp;
	while(node!=NULL)
	{
		tmp = node;
		node = node->next;
		free(tmp);
	}
}

/* Function to insert a node at the beginging of the linked list */
void push(struct node **head_ref, int new_data)
{
  /* allocate node */
  struct node *new_node =
            (struct node*) malloc(sizeof(struct node));

  /* put in the data  */
  new_node->data  = new_data;

  /* link the old list off the new node */
  new_node->next = (*head_ref);

  /* move the head to point to the new node */
  (*head_ref)    = new_node;
}


/* Drier program to test above functions*/
int main()
{
	/* Start with the empty list */
	node_t* a = NULL;
	int i = 0;
	divided_count = 1;

	/*init time variable*/
	struct timeval start, end;

	/*init pthread*/
	if ((threads = (pthread_t *) (malloc(max_thread_num * sizeof(pthread_t)))) == NULL){
		fprintf(stderr, "alloc thread fail\n");
		exit(1);
	}
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&gmutex, NULL);
	thread_count = 1;
	divided_table = malloc(sizeof(node_t*)*divided_remalloc_base);

	//create linked list with random numbers;
	srand(time(NULL));
	for (i=0; i<size; i++){
		push(&a, (rand())%range);
	}

#ifdef dump_data
	 // dump unsorted element.
	printf("\n Non Sorted Linked List is: \n");
	printList(a);           
#endif
	gettimeofday(&start, NULL); 

	/* Sort the above created Linked List */
	if(thread_count < max_thread_num){
		if(pthread_create(&threads[thread_count++], &attr, MT_MergeSort, (void*)(&a)) != 0){
			fprintf(stderr, "create thread error\n");
			exit(1);
		}
		pthread_join(threads[1], NULL);
		merge_table();
	}else{
		ST_MergeSort(&a);
	}

	gettimeofday(&end, NULL); 

    // compute the exact duration of the experiment
    duration = (end.tv_sec * 1000 + end.tv_usec / 1000) -
               (start.tv_sec * 1000 + start.tv_usec / 1000);

#ifdef dumo_data
	// dump sorted element
	printf("\n Sorted Linked List is: \n");
	if(max_thread_num == 1){
		printList(a);           
	}else{
		printList(divided_table[0]);           
	}
#endif

	printf("duration : %d (ms)\n", duration);

	freeList(divided_table[0]);
	free(divided_table);

	return 0;
}
