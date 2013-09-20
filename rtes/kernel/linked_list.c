/*Our custom linked list*/
void add_node(NODE** head, NODE* curr1)
{
	NODE* curr2 = *head;

	if (!curr1)
	{
		return;
	}

	if (*head == NULL)
	{
		*head = curr1;
	}
	else
	{
		if (curr2->prio <= curr1->prio)
		{
			curr1->next = curr2;
			*head = curr1;
		}
		else
		{
			while (curr2->prio > curr1->prio || curr2->next)
				curr2 = curr2->next;

			if (curr2->next)
			{
				NODE* temp = curr2->next;
				curr1->next = temp;
			}
			curr2->next = curr1;
		}
	}
}

void delete_linked_list (NODE** head)
{
	NODE* curr = *head;

	while (curr)
	{
		NODE* temp = curr;
		curr = curr->next;
		free(temp);
		temp = NULL;
	}
}
