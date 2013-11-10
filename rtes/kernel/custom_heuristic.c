#include <linux/types.h>
#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/reserve_framework.h>
#include <linux/bin_linked_list.h>
//#define ARRAYSIZE(a) (sizeof(a))/(sizeof(a[0]))

void find_combinations(int sub_pa_length);

extern BIN_NODE* bin_head;
BIN_NODE* pa_head = NULL;
BIN_NODE* sub_pa_head = NULL;
BIN_NODE* sub_pa_tail = NULL;
static int total_nodes;

struct period_length{

	unsigned long long period;
	int length;
};

/*
 *print_pa_list: Function used to debug along the way.
 */
void print_pa_list(BIN_NODE* head){

	BIN_NODE* curr = head;
	while (curr){

		printk(KERN_INFO "C = %llu T = %llu", timespec_to_ns(&curr->task->reserve_process.C), timespec_to_ns(&curr->task->reserve_process.T));
		printk(KERN_INFO "PID = %d U =%llu", curr->task->pid, curr->task->reserve_process.U);
		curr = curr->next;
	}
	printk("\n");

}
/*
 * make_pa_node: Makes a new node to be attached shortly
 */
BIN_NODE* make_pa_node(struct task_struct *task)
{
	BIN_NODE* node = kmalloc(sizeof(BIN_NODE), GFP_KERNEL);
	node->task = task;
	node->next = NULL;
	return node;
}

/*
 * add_pa_node: Add a bin_node in a sorted fashion according to Time Period
 */

void add_pa_node(BIN_NODE* curr1)
{
	BIN_NODE* curr2 = pa_head;
	BIN_NODE* temp = NULL;

	if (!curr1)
	{
		return;
	}

	if (pa_head == NULL)
	{
//		printk(KERN_INFO "PA Head Created");
		pa_head = curr1;
	}
	else
	{
		if (timespec_to_ns(&curr2->task->reserve_process.T) > timespec_to_ns(&curr1->task->reserve_process.T))
		{
			curr1->next = curr2;
			pa_head = curr1;
		}
		else
		{
			while( curr2 && (timespec_to_ns(&curr2->task->reserve_process.T) <= timespec_to_ns(&curr1->task->reserve_process.T)))
			{
				temp = curr2;
				curr2 = curr2->next;
			}

			if(curr2)
			{
				curr1->next = curr2;
			}
			temp->next = curr1;
		}
	}
}

BIN_NODE* find_pa_node (unsigned long long base_period){

	BIN_NODE* curr = pa_head;

	while (curr && timespec_to_ns(&curr->task->reserve_process.T) != base_period){
		curr =  curr->next;
	}

	if (curr == NULL)
		curr =  NULL;

	return curr;
}

/*
 * delete_pa_node: Delete a bin_node
 */
void delete_pa_node (struct task_struct *task)
{
	BIN_NODE* curr = pa_head;
	BIN_NODE* prev = NULL;

	while (curr)
	{
		if (curr->task == task)
		{
			if (curr == pa_head)
			{
				pa_head = curr->next;
				kfree(curr);
			}
			else
			{
				prev->next = curr->next;
				kfree(curr);
			}
			curr = NULL;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}

/*
 * add_sub_pa_node: Add a node to the best linked list
 */
void add_sub_pa_node(BIN_NODE* curr)
{
	if (sub_pa_head == NULL){
		sub_pa_head = curr;
	}
	else{
		sub_pa_tail->next = curr;
	}
	sub_pa_tail = curr;

}

/*
 * del_sub_pa_node: Add a node to the best linked list
 */
/*void del_sub_pa_node(BIN_NODE* curr)
{
	BIN_NODE* temp = sub_pa_head;
	BIN_NODE* prev = NULL;
	if (curr == sub_pa_head){
		sub_pa_head = curr->next;
		curr = NULL;
		kfree(curr);
	}
	else{
		while(temp){
			if (temp == curr){
				prev->next = curr->next;
				temp = NULL;
				kfree(temp);
			}
			prev = temp;
			temp = temp->next;
	}

}*/

/*
 *
 */
struct period_length* initialize_tasks(struct period_length* p_len, int pa_length){

	int i = 0;
	for (i = 0; i < pa_length; i++){
		p_len[i].period = 0;
		p_len[i].length = 0;
	}

	return p_len;

}

/*
 * eratosthenes_sieve: Creates multiple linked lists of harmonic tasks.
 * The remaining nodes in the original linked list do not belong to any
 * harmonic period.
 */
void eratosthenes_sieve(struct period_length* p_len){

	BIN_NODE* outer_curr = pa_head;
	BIN_NODE* inner_curr;
	uint64_t T_var = 0;
	uint64_t* T_temp = (uint64_t *)&T_var;
	uint32_t remainder = 0;
	unsigned long long base_period;
	int i = 0;

	while (outer_curr){

		inner_curr = pa_head;
		base_period = timespec_to_ns(&inner_curr->task->reserve_process.T);
		p_len[i].period = base_period;
		printk(KERN_INFO "Period = %llu\n", p_len[i].period);

		while (inner_curr){

			*T_temp = timespec_to_ns(&inner_curr->task->reserve_process.T);
			remainder = do_div(*T_temp, base_period);

			if(timespec_to_ns(&inner_curr->task->reserve_process.T) == base_period || remainder == 0){
				printk(KERN_INFO "Multiple was %llu\n",timespec_to_ns(&inner_curr->task->reserve_process.T));
				p_len[i].length++;
			}

			inner_curr = inner_curr->next;
		}
		i++;
		outer_curr = outer_curr->next;
	}

}

struct period_length* find_max_p_length(struct period_length* p_len, int pa_length){

	int i = 0;
	int max_length = p_len[i].length;
	struct period_length* result = p_len;

	for (i = 1; i < pa_length; i++){

		if (p_len[i].length > max_length){
			result = p_len;
		}
	}
	return result;
}

/*
 * find_length: Trivial function that finds the length of the linked list
 */
int find_length(BIN_NODE* head){

	int length = 0;
	BIN_NODE* curr = head;

	while (curr){
		length++;
		curr = curr->next;
	}
	return length;
}

//Insert Combinations Code

void apply_custom_fit(void){

	int sub_pa_length = 0;
	int pa_length = 0;
	BIN_NODE* curr = bin_head;
	BIN_NODE* base = NULL;
	struct period_length* p_len  = NULL;
	struct period_length* base_period = NULL;
	uint64_t T_var = 0;
	uint64_t* T_temp = (uint64_t *)&T_var;
	uint32_t remainder = 0;

	while (curr){

		add_pa_node(make_pa_node(curr->task));
		curr = curr->next;
	}
	print_pa_list(pa_head);

	pa_length = find_length(pa_head);
	printk("Length of the List is %d\n", pa_length);

	p_len = kmalloc(sizeof(struct period_length) * pa_length, GFP_KERNEL);
	initialize_tasks(p_len, pa_length);
	eratosthenes_sieve(p_len);
	base_period = find_max_p_length(p_len, pa_length);
	printk(KERN_INFO "Maximum Length of Task List = %d\n", base_period->length);
	base = find_pa_node(base_period->period);

	while (base){
		*T_temp = timespec_to_ns(&base->task->reserve_process.T);
		remainder = do_div(*T_temp, base_period->period);
		if(timespec_to_ns(&base->task->reserve_process.T) == base_period->period || remainder == 0){
			add_sub_pa_node(make_pa_node(base->task));
		}
		base = base->next;
	}

	print_pa_list(sub_pa_head);
	sub_pa_length = find_length(sub_pa_head);
	printk("Length of the New List is %d\n", sub_pa_length);
	find_combinations(sub_pa_length);

}

// prints subset found
void printSubset(unsigned long long A[], unsigned long long size)
{
	int i = 0;
	for(i = 0; i < size; i++)
	{
		printk("%*llu", 5, A[i]);
	}
	printk("\n");

	//Add it to the new CPU
	//Delete that Node from that Sub_PA_List
	//Make that node in the array 0
}

// qsort compare function
int comparator(const void *pLhs, const void *pRhs)
{
	unsigned long long *lhs = (unsigned long long *)pLhs;
	unsigned long long *rhs = (unsigned long long *)pRhs;

	return *lhs > *rhs;
}

// inputs
// s            - set vector
// t            - tuplet vector
// s_size       - set size
// t_size       - tuplet size so far
// sum          - sum so far
// ite          - nodes count
// target_sum   - sum to be found
void subset_sum(unsigned long long s[], unsigned long long t[],
		int s_size, int t_size,
		unsigned long long sum, unsigned long long ite,
		unsigned long long const target_sum)
{
	int i = 0;
	total_nodes++;

	if( target_sum == sum )
	{
		// We found sum
		printSubset(t, t_size);

		// constraint check
		if( ite + 1 < s_size && sum - s[ite] + s[ite+1] <= target_sum )
		{
			// Exclude previous added item and consider next candidate
			subset_sum(s, t, s_size, t_size-1, sum - s[ite], ite + 1, target_sum);
		}
		return;
	}
	else
	{
		// constraint check
		if( ite < s_size && sum + s[ite] <= target_sum )
		{
			// generate nodes along the breadth
			for(i = ite; i < s_size; i++ )
			{
				t[t_size] = s[i];

				if( sum + s[i] <= target_sum )
				{
					// consider next level node (along depth)
					subset_sum(s, t, s_size, t_size + 1, sum + s[i], i + 1, target_sum);
				}
			}
		}
	}
}

// Wrapper that prints subsets that sum to target_sum
void generateSubsets(unsigned long long s[], int size, int target_sum)
{
	unsigned long long *tuplet_vector = (unsigned long long *)kmalloc(size * sizeof(unsigned long long), GFP_KERNEL);

	unsigned long long total = 0;
	int i = 0;

	// sort the set
	sort(s, size, sizeof(unsigned long long), &comparator, NULL);

	for(i = 0; i < size; i++ )
	{
		total += s[i];
	}

	if( s[0] <= target_sum && total >= target_sum )
	{

		subset_sum(s, tuplet_vector, size, 0, 0, 0, target_sum);

	}

	kfree(tuplet_vector);
}

void find_combinations(int sub_pa_length){
	int i = 0;
	unsigned long long *weights;
	unsigned long long target = 0;
	int size = 0;
	BIN_NODE* curr = sub_pa_head;

	weights = (unsigned long long *)kmalloc(sizeof(unsigned long long)*sub_pa_length, GFP_KERNEL);

	while(curr){
		weights[i] = curr->task->reserve_process.U;
		curr = curr->next;
		i++;
	}

	for (i = 0; i < sub_pa_length; i++){
		printk(KERN_INFO "Weight [%d] = %llu", i, weights[i]);
	}
//	size = ARRAYSIZE(weights);
	size = sub_pa_length;
	i = 0;
//	for (i = 0; i < sub_pa_length; i++){
		target = 10000 - weights[i];
		printk(KERN_INFO "Size = %d, Target = %llu\n", size, target);
		generateSubsets(weights, size, target);
//		weights++;
//		size = ARRAYSIZE(weights);
//	}

}

