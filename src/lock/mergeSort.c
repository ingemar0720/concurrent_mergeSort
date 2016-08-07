#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#define size 100000
#define range 200000

/* Link list node */
struct node
{
	int data;
	struct node *next;
};

/* function prototypes */
struct node* SortedMerge(struct node *a, struct node *b);
void FrontBackSplit(struct node *source,
		struct node **frontRef, struct node **backRef);

/* sorts the linked list by changing next pointers (not data) */
void MergeSort(struct node **headRef)
{
	struct node *head = *headRef;
	struct node *a;
	struct node *b;

	/* Base case -- length 0 or 1 */
	if ((head == NULL) || (head->next == NULL))
	{
		return;
	}

	/* Split head into 'a' and 'b' sublists */
	FrontBackSplit(head, &a, &b); 

	/* Recursively sort the sublists */
	MergeSort(&a);
	MergeSort(&b);

	/* answer = merge the two sorted lists together */
	*headRef = SortedMerge(a, b);
}

/* See http://geeksforgeeks.org/?p=3622 for details of this 
   function */
struct node* SortedMerge(struct node *a, struct node *b)
{
	struct node *result = NULL;

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
void FrontBackSplit(struct node *source,
		struct node **frontRef, struct node **backRef)
{
	struct node *fast;
	struct node *slow;
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
void printList(struct node *node)
{
	while(node!=NULL)
	{
		printf("%d\n ", node->data);
		node = node->next;
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
	struct node* a = NULL;
	int i = 0;
	struct timeval start, end;
	int duration = 0;

	//create linked list with random numbers;
	srand(time(NULL));
	for (i=0; i<size; i++){
		push(&a, (rand())%range);
	}

	/* Sort the above created Linked List */
	gettimeofday(&start, NULL);
	MergeSort(&a);
	gettimeofday(&end, NULL);

	printf("\n Sorted Linked List is: \n");
	printList(a);           
    // compute the exact duration of the experiment
    duration = (end.tv_sec * 1000 + end.tv_usec / 1000) -
               (start.tv_sec * 1000 + start.tv_usec / 1000);

    printf("duration : %d (ms)\n", duration);

	return 0;
}