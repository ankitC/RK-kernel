#include <linux/types.h>
#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/reserve_framework.h>
#include <linux/bin_linked_list.h>
#include <linux/cpu_linked_list.h>
#include <linux/partition_scheduling.h>
#define TOTAL_CORES 4
#define UNSCHEDULABLE 2

int find_combinations(int sub_pa_length);
int apply_first_fit_pa(void);
void del_all_sub_pa_nodes(void);
void del_all_pa_nodes(void);

extern const uint32_t bounds_tasks[62];
extern BIN_NODE* cpu_bin_head[4];
extern BIN_NODE* bin_head;
BIN_NODE* pa_head = NULL;
BIN_NODE* sub_pa_head = NULL;
BIN_NODE* sub_pa_tail = NULL;

static int total_nodes;
int iter = 0;

struct w{

	unsigned long long weights;
	int pid;
};

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

		printk(KERN_INFO "Enters the print_pa_list function\n");
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

/*
 * find_pa_node: Find PA node by Period
 */
BIN_NODE* find_pa_node (unsigned long long base_period){

	BIN_NODE* curr = pa_head;

	if (timespec_to_ns(&curr->task->reserve_process.T) == base_period)
		return curr;

	while (curr && timespec_to_ns(&curr->task->reserve_process.T) != base_period){
		curr =  curr->next;
	}

	if (curr == NULL)
		curr =  NULL;

	return curr;
}

/*
 * find_pa_node_pid: Find PA node by PID
 */
BIN_NODE* find_pa_node_pid (int pid){

	BIN_NODE* curr = pa_head;
	if (curr->task->pid == pid)
		return curr;

	while (curr && curr->task->pid != pid){
		curr =  curr->next;
	}

	if (curr == NULL){
		curr =  NULL;
	}

	return curr;
}

/*
 * delete_pa_node: Delete a PA node
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
 * del_all_pa_nodes: Deletes all the nodes in the PA list
 */
void del_all_pa_nodes(void){

	BIN_NODE* curr = pa_head;
	BIN_NODE* prev = NULL;

	while(curr){

		prev = curr;
		curr = curr->next;
		kfree(prev);
		prev = NULL;
	}
	pa_head = NULL;
}

/*
 * FrontBackSplit: finds the middle of the linked list
 */
void FrontBackSplit(BIN_NODE* source, BIN_NODE** frontRef, BIN_NODE** backRef)
{
	BIN_NODE* fast;
	BIN_NODE* slow;
	if (source == NULL || source->next == NULL)
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
		 *       at that point. */
		*frontRef = source;
		*backRef = slow->next;
		slow->next = NULL;
	}
}

/* SortedMerge: Merge Sort for the PA Linked List*/
BIN_NODE* SortedMerge(BIN_NODE* a, BIN_NODE* b)
{
	BIN_NODE* result = NULL;

	/* Base cases */
	if (a == NULL)
		return(b);
	else if (b == NULL)
		return(a);

	/* Pick either a or b, and recur */
	if (a->task->reserve_process.U >= b->task->reserve_process.U)
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

/* sorts the linked list by changing next pointers (not data) */
void MergeSort(BIN_NODE** headRef)
{
	BIN_NODE* head = *headRef;
	BIN_NODE* a;
	BIN_NODE* b;

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
 * del_all_sub_pa_nodes: Deletes all the nodes in the harmonic linked list
 */
void del_all_sub_pa_nodes(void){

	BIN_NODE* curr = sub_pa_head;
	BIN_NODE* prev = NULL;

	while(curr){

		prev = curr;
		curr = curr->next;
		kfree(prev);
		prev = NULL;
	}
	sub_pa_head = NULL;
	sub_pa_tail = NULL;

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
	unsigned long long base_period = 0;
	uint64_t B_var = 0;
	uint64_t* B_temp = (uint64_t *)&B_var;
	uint32_t scaled_base_period = 0;
	int i = 0;

	while (outer_curr){

		inner_curr = outer_curr;
		base_period = timespec_to_ns(&inner_curr->task->reserve_process.T);
		p_len[i].period = base_period;
		printk(KERN_INFO "Period = %llu\n", p_len[i].period);

		while (inner_curr){

			*B_temp = base_period;
			scaled_base_period = do_div(*B_temp, 10000);
			*T_temp = timespec_to_ns(&inner_curr->task->reserve_process.T);
			printk(KERN_INFO "Period of this node = %llu\n", *T_temp);
			remainder = do_div(*T_temp, *B_temp);
			printk(KERN_INFO "Remainder = %d\n", remainder);

			if(timespec_to_ns(&inner_curr->task->reserve_process.T) == base_period || remainder == 0){
//				printk(KERN_INFO "Multiple was %llu\n",timespec_to_ns(&inner_curr->task->reserve_process.T));
				p_len[i].length++;
			}

			inner_curr = inner_curr->next;
		}
		i++;
		outer_curr = outer_curr->next;
	}

}

/*
 * find_max_p_length: Find the Period that dominates your task set
 */
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
 * find_length: Finds the length of the linked list
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

int apply_custom_fit(void){

	int sub_pa_length = 0;
	int pa_length = 0;
	BIN_NODE* curr = bin_head;
	BIN_NODE* base = NULL;
	struct period_length* p_len  = NULL;
	struct period_length* base_period = NULL;
	uint64_t T_var = 0;
	uint64_t* T_temp = (uint64_t *)&T_var;
	uint32_t remainder = 0;
	uint64_t B_var = 0;
	uint64_t* B_temp = (uint64_t *)&B_var;
	uint32_t scaled_base_period = 0;
	int retval = 0;
	iter = 0;

	while (curr){

		add_pa_node(make_pa_node(curr->task));
		curr = curr->next;
	}
	print_pa_list(pa_head);

	pa_length = find_length(pa_head);

	printk("Length of the List is %d\n", pa_length);
	if (pa_length == 0)
		return 1;

	p_len = kzalloc(sizeof(struct period_length) * pa_length, GFP_KERNEL);
	eratosthenes_sieve(p_len);
	base_period = find_max_p_length(p_len, pa_length);
	printk(KERN_INFO "Maximum Length of Task List = %d\n", base_period->length);
	base = find_pa_node(base_period->period);

	while (base){
		printk(KERN_INFO "Creating a second LL\n");
		*B_temp = timespec_to_ns(&base->task->reserve_process.T);
		scaled_base_period = do_div(*B_temp, 10000);
		*T_temp = timespec_to_ns(&base->task->reserve_process.T);
		remainder = do_div(*T_temp, *B_temp);
		if(timespec_to_ns(&base->task->reserve_process.T) == base_period->period || remainder == 0){
			add_sub_pa_node(make_pa_node(base->task));
		}
		base = base->next;
	}

	print_pa_list(sub_pa_head);
	sub_pa_length = find_length(sub_pa_head);
	printk("Length of the New List is %d\n", sub_pa_length);
	retval = find_combinations(sub_pa_length);
	return retval;
}

void assign_cpus(struct w A[], int size, int iter){

	int i = 0;
	BIN_NODE* curr = NULL;

	for (i = 0; i < size; i++){
		curr = find_pa_node_pid(A[i].pid);
		curr->task->reserve_process.prev_cpu = curr->task->reserve_process.host_cpu;
		curr->task->reserve_process.host_cpu = iter;
		printk(KERN_INFO "PID %d goes to CPU %d\n", A[i].pid, iter);
		delete_pa_node(curr->task);
	}
}

// prints subset found
//void printSubset(unsigned long long A[], unsigned long long size)
int printSubset(struct w A[], int size)
{
	int i = 0;

	BIN_NODE* curr = NULL;
	printk("Entered PrintSubset\n");

	for(i = 0; i < size; i++)
	{
		printk("%*llu", 5, A[i].weights);
		printk("%*d", 5, A[i].pid);
		curr = find_pa_node_pid(A[i].pid);
		if (!curr){
			printk(KERN_INFO "Discarded Combination\n");
			return 1;
		}
	}
	printk("\n");

	if (iter == 0){
		assign_cpus(A, size, iter);
		iter++;
		return 1;
	}
	else{
		if (iter < 4){
			assign_cpus(A, size, iter);
			iter++;
			return 1;
		}
		else{
			printk(KERN_INFO "Need to return an error here\n");
			return -1;
		}
	}
}

/*
 * sort_util: Sorts the array according to Utilization
 */
void sort_util(struct w s[], int size){

	int i, j;
	struct w temp;

	for(i = 0; i < size; i++)
	{
		for(j = i; j < size; j++)
		{
			if(s[i].weights > s[j].weights)
			{
				temp = s[i];
				s[i] = s[j];
				s[j] = temp;
			}
		}
	}

}

// inputs
// s            - set vector
// t            - tuplet vector
// s_size       - set size
// t_size       - tuplet size so far
// sum          - sum so far
// ite          - nodes count
// target_sum   - sum to be found
/*void subset_sum(unsigned long long s[], unsigned long long t[],
		int s_size, int t_size,
		unsigned long long sum, unsigned long long ite,
		unsigned long long const target_sum)*/
int subset_sum(struct w s[], struct w t[],
		int s_size, int t_size,
		unsigned long long sum, unsigned long long ite,
		unsigned long long const target_sum)
{
	int i = 0;
	static int retval = 0;
	total_nodes++;

	if( target_sum == sum )
	{
		// We found sum
		retval |= printSubset(t, t_size); // <--- Check if this returns -1

		// constraint check
//		if( ite + 1 < s_size && sum - s[ite] + s[ite+1] <= target_sum )
		if( ite + 1 < s_size && sum - s[ite].weights + s[ite+1].weights <= target_sum )
		{
			// Exclude previous added item and consider next candidate
//			subset_sum(s, t, s_size, t_size-1, sum - s[ite], ite + 1, target_sum);
			subset_sum(s, t, s_size, t_size-1, sum - s[ite].weights, ite + 1, target_sum);
		}
//		return retval;
	}
	else
	{
		// constraint check
//		if( ite < s_size && sum + s[ite] <= target_sum )
		if( ite < s_size && sum + s[ite].weights <= target_sum )
		{
			// generate nodes along the breadth
			for(i = ite; i < s_size; i++ )
			{
//				t[t_size] = s[i];
				t[t_size].weights = s[i].weights;
				t[t_size].pid = s[i].pid;

//				if( sum + s[i] <= target_sum )
				if( sum + s[i].weights <= target_sum )
				{
					// consider next level node (along depth)
//					subset_sum(s, t, s_size, t_size + 1, sum + s[i], i + 1, target_sum);
					subset_sum(s, t, s_size, t_size + 1, sum + s[i].weights, i + 1, target_sum);
				}
			}
		}
	}
	printk(KERN_INFO "subset sum returns %d\n", retval);
	return retval;
}

// Wrapper that prints subsets that sum to target_sum
int generateSubsets(struct w s[], int size, int target_sum)
{
//	unsigned long long *tuplet_vector = (unsigned long long *)kmalloc(size * sizeof(unsigned long long), GFP_KERNEL);
	struct w *tuplet_vector = (struct w *)kzalloc(size * sizeof(struct w), GFP_KERNEL);

	unsigned long long total = 0;
	int i = 0;
	int retval = 0;

	// sort the set
	sort_util(s, size);
	printk(KERN_INFO "Target = %d\n", target_sum);

	for(i = 0; i < size; i++ )
	{
		printk(KERN_INFO "[%d] U = %llu", i, s[i].weights);
		printk(KERN_INFO "[%d] PID = %d", i, s[i].pid);
	}

	for(i = 0; i < size; i++){

		total += s[i].weights;

	}

//	if( s[0] <= target_sum && total >= target_sum )
	if( s[0].weights <= target_sum && total >= target_sum )
	{

		 retval = subset_sum(s, tuplet_vector, size, 0, 0, 0, target_sum);

	}

	kfree(tuplet_vector);
	printk(KERN_INFO "generate Subsets returns %d\n", retval);
	return retval;
}

int find_combinations(int sub_pa_length){
	int i = 0;
//	unsigned long long *weights;
	struct w *wts;
	unsigned long long target = 0;
	int retval_p = 0;
	int retval_f = 0;
	int size = 0;
	BIN_NODE* curr = sub_pa_head;

//	weights = (unsigned long long *)kmalloc(sizeof(unsigned long long)*sub_pa_length, GFP_KERNEL);
	wts = (struct w *)kzalloc(sizeof(struct w)*sub_pa_length, GFP_KERNEL);

	while(curr){
//		weights[i] = curr->task->reserve_process.U;
		wts[i].weights = curr->task->reserve_process.U;
		wts[i].pid = curr->task->pid;
		curr = curr->next;
		i++;
	}

	for (i = 0; i < sub_pa_length; i++){
		printk(KERN_INFO "Weight [%d] = %llu", i, wts[i].weights);
		printk(KERN_INFO "PID [%d] = %d", i, wts[i].pid);
	}

	size = sub_pa_length;
	i = 0;
	target = 10000;
//	generateSubsets(weights, size, target);
	retval_p = generateSubsets(wts, size, target);
	printk(KERN_INFO "find combinations returns %d\n", retval_p);

	//Delete all nodes from sub_pa_list
	del_all_sub_pa_nodes();
	//Call Best Fit on the remaining nodes of pa_list
	if (pa_head){
		MergeSort(&pa_head);
		retval_f = apply_first_fit_pa();
	}
	//Delete all nodes of pa_list
	del_all_pa_nodes();
	//Determine the return value
	if ((retval_p == 1 && retval_f == 1) || (retval_p == 1 && retval_f == 0) || (retval_p == 0 && retval_f == 1))
		return 1;
	else
		return -1;
}

/*
 * Utilization bound test to check for a task
 * Returns UNSCHEDULABLE on Failure.
 * Returns 0 when RT test is required.
 * Returns 1 on Success.
 */
int ub_cpu_test_pa(BIN_NODE *curr1, int cpu)
{

	BIN_NODE *curr = cpu_bin_head[cpu];
	unsigned int i = 0;
	unsigned long long total_util = curr1->task->reserve_process.U;

	printk(KERN_INFO "ub test: curr1 util %llu\n", curr1->task->reserve_process.U);

	if (cpu_bin_head[cpu] == NULL && curr1->task->reserve_process.U < bounds_tasks[0])
	{
		printk(KERN_INFO "CPU BIN HEAD NULL\n");
		add_cpu_node(make_cpu_node(curr1->task), cpu);
		return 1;
	}

	while(curr)
	{

		total_util += curr->task->reserve_process.U;
		curr = curr->next;
		i++;
	}

	if (total_util	> bounds_tasks[0])
		return UNSCHEDULABLE;
	if(total_util > bounds_tasks[i])
		return 0;

	add_cpu_node(make_cpu_node(curr1->task), cpu);
	return 1;
}
/*
 * Calculation for RT-Test on per task basis.
 * Returns 1 on Success and 0 on failure
 */
int check_cpu_schedulabilty_pa(BIN_NODE *stop, int cpu)
{
	unsigned long long a[24] = {0};
	int i = 0;

	uint64_t A_var = 0;
	uint64_t T_var = 0;
	uint64_t* A_temp = (uint64_t*)&A_var;
	uint64_t* T_temp = (uint64_t*)&T_var;
	uint32_t remainder= 0, t = 0;
	BIN_NODE *curr = cpu_bin_head[cpu];

	while (curr != stop->next)
	{
		a[0] += timespec_to_ns(&curr->task->reserve_process.C);
		curr = curr->next;
	}

	curr = cpu_bin_head[cpu];

	while (1)
	{
		a[i + 1] = 0;
		while (curr != stop)
		{

			*A_temp = a[i];
			*T_temp = timespec_to_ns(&curr->task->reserve_process.T);
			//printk(KERN_INFO "Before: *A_temp = %llu", *A_temp);
			//printk(KERN_INFO "Before: *T_temp = %llu", *T_temp);
			remainder = do_div(*T_temp, 10000);
			t = (uint32_t) *T_temp;

			/* Calculating Ceiling */
			remainder = do_div(*A_temp, t);
			if ((remainder = do_div(*A_temp, 10000)) > 0)
			{
				*A_temp = *A_temp + 1;
				//	printk(KERN_INFO "After: *A_temp = %llu", *A_temp);

			}
			//	printk(KERN_INFO "After Scaling: *A_temp = %llu", *A_temp);

			a[i + 1] += *A_temp * timespec_to_ns(&curr->task->reserve_process.C);
			curr = curr->next;
		}

		a[i + 1] += timespec_to_ns(&stop->task->reserve_process.C);

		if ( a[i] == a[i + 1]){
			printk(KERN_INFO "RT Test succeeds a[%d] = %llu", i, a[i]);
			return 1;
		}

		if (a[i + 1] > timespec_to_ns(&stop->task->reserve_process.T))
		{
			printk(KERN_INFO " RT Test failed a[%d] = %llu", i + 1, a[i]);
			printk(KERN_INFO "T = %llu", timespec_to_ns(&stop->task->reserve_process.T));
			return 0;
		}

		i++;
		curr = cpu_bin_head[cpu];
		*A_temp = 0;
		*T_temp = 0;
	}

	return 0;
}

/*
 * Response time test to check whether the task is schedulable on that cpu
 * Returns 1 for success and 0 for failure
 */
int rt_cpu_test_pa(BIN_NODE* foo, int cpu)
{
	unsigned long long total_util = 0;
	int k = 0;
	BIN_NODE *curr = cpu_bin_head[cpu];
	int j = 0;

//	printk(KERN_INFO "RT Test\n");
	add_cpu_node(make_cpu_node(foo->task), cpu);

	while (curr)
	{
		total_util += curr->task->reserve_process.U;
		if (total_util > bounds_tasks[j]){
			break;
		}
		else
			j++; // position in linked list where the RT test has to be started
		curr = curr->next;
	}

	curr = cpu_bin_head[cpu];

	/* Forwarding the linkedlist upto the position*/
	for (k = 0; k < j; k++)
		curr = curr->next;

	while (curr)
	{
		if(check_cpu_schedulabilty_pa(curr, cpu)){
//			printk(KERN_INFO "We have another task to run this test on\n");
			curr = curr->next;
		}
		else{
			printk(KERN_INFO "End of Linked List\n");
			return 0;
		}
	}

	return 1;
}

/*
 * Admission test for a particular cpu
 */
int admission_test_for_cpu_pa(BIN_NODE* curr, int cpu)
{
	int retval = 0;

	retval = ub_cpu_test_pa(curr, cpu);

	if (retval == UNSCHEDULABLE)
		return -1;

	if (retval || rt_cpu_test_pa(curr, cpu))
	{
		return 1;
	}
	else
	{
		delete_cpu_node(curr->task, cpu); // <--- Should this be here?!
		return -1;
	}
}
/*
 * First fit heuristic for remaining pa_nodes
 */
int apply_first_fit_pa(void)
{
	int cpu = iter;
	BIN_NODE* curr = pa_head;

	printk(KERN_INFO "First fit to remaining nodes\n");
	while (curr && cpu < TOTAL_CORES)
	{
		if (admission_test_for_cpu_pa(curr, cpu) < 0)
		{
			cpu++;
		}
		else
		{
			printk(KERN_INFO "Setting cpu %d", cpu);
			curr->task->reserve_process.prev_cpu = curr->task->reserve_process.host_cpu;
			curr->task->reserve_process.host_cpu = cpu;
			curr = curr->next;
			if (curr)
				printk(KERN_INFO "Next node exists\n");
			cpu = 0;
		}
	}

	if (cpu == TOTAL_CORES)
		return -1;
	else
		return 1;
}

