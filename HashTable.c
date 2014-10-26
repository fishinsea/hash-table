#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "HashTable.h"
struct HashTableObjectTag
{
    int sentinel;
    unsigned int size; // number of buckets
	unsigned int elements; // total number of entries in hashtable
	unsigned int *bucket_sizes; //counter for how many entries in each bucket
	void **head_array; // array of linked lists, void ** because struct not defined in .h
	int dynamic; // stores the dynamicm behaviour parameters
	float expand;
	float contract;
};


typedef struct binary_tree
{
	void *data;
	char *key;
	struct binary_tree *left;
	struct binary_tree *right;
} bucket;

static unsigned char SimpleIntHash( int value, unsigned char range )
{
	int result;
	result = ( value % range );
	if ( result < 0 )
	{
		result = -result;
	}
	return (unsigned char) result;
}

static unsigned char SimpleStringHash( char *value, unsigned char range )
{
	int total = 0;
	for (int i = 0; i < strlen( value ); i++ )
	{
		total = total + (int) value[i];
	}
	return SimpleIntHash( total, range ); 
}

static bucket *NewNode(char *key, void *data)
{ //function to create a new node in the linked list
	bucket *new = malloc(sizeof(bucket));
	new->key = key;
	new->data = data;
	new->left = NULL;
	new->right = NULL;
	return new;
}

static void FreeBinaryNode(bucket* node_ptr)
{ // frees node in a binary tree and all its children
	if (node_ptr == NULL)
		return;
	free((node_ptr)->key);
	if ((node_ptr)->left == NULL && (node_ptr)->right == NULL)
	{ // if both children are NULL, frees this node
		bucket *temp = node_ptr;
		node_ptr = NULL;
		free(temp);
	} // else calls the same function on its children
	else
	{
		FreeBinaryNode((node_ptr)->left);
		FreeBinaryNode((node_ptr)->right);
		free(node_ptr);
	}
}

static void RESIZE (HashTablePTR hashTable, int expand)
{
	hashTable->dynamic = 0; // temp. set dynamic to 0 for insertion
	unsigned int num_of_entries = hashTable->elements;
	
	char **keys_array; void **data_array; unsigned int key_count; void *prev_data;
	data_array = malloc(sizeof(void*) * num_of_entries); // temp array of keys and data
	
	GetKeys(hashTable, &keys_array, &key_count); // copies the keys 
	
	for (int i = 0; i < key_count; i++)
	{ // findentry copies the data into the data array
		FindEntry(hashTable, keys_array[i], &(data_array[i]));
		DeleteEntry(hashTable, keys_array[i], &prev_data); //deletes the entry after copying
	}

	free(hashTable->head_array); //frees and resets the head_array
	
	if (expand == 1) // increase/decrease size of hashTable by 1
		hashTable->size++;
	if (expand == -1)
		hashTable->size--;
		
	hashTable->head_array = malloc(sizeof(bucket *) * hashTable->size);
	free(hashTable->bucket_sizes);
	(hashTable)->bucket_sizes = calloc(hashTable->size, sizeof(int));
	(hashTable)->elements = 0;
	
	for (int i = 0; i < hashTable->size; i++)
	{ //head_array is an array of pointers to nodes
		(((hashTable))->head_array)[i] = NULL;//initializes head pointer to null
	}
	for (int i = 0; i < key_count; i++)
	{ // re-allocate memory and insert all the key/data pairs, and frees them after
		InsertEntry(hashTable, keys_array[i], data_array[i], &prev_data);
		free(keys_array[i]);
	}
	free(keys_array);
	free(data_array);
	// change dynamic back to 1
	hashTable->dynamic = 1;
}

int checkRESIZE (HashTablePTR hashTable)
{
	if (*((int*)(hashTable)) != 0xDEADBEEF)
		return -1;
	// checks whether pointer is a hashtable and whether expansion is needed
	
	if (hashTable->dynamic == 1)
	{ // checks whether resizing is necessary
		int expand = 0;
		HashTableInfo *info = malloc(sizeof(HashTableInfo));
		GetHashTableInfo(hashTable, info);
		if (info->useFactor < info->contractUseFactor)
			expand = -1;
		else if (info->useFactor > info->expandUseFactor)
			expand = 1;
		if (expand != 0)
		{
			RESIZE(hashTable, expand);
			free(info);
			return 0;
		}
		free(info);
	}
	return -1;
}

int CreateHashTable( HashTablePTR *hashTableHandle, unsigned int initialSize )
{
	if ((*(hashTableHandle)) != NULL)
		DestroyHashTable(hashTableHandle); 
	(*(hashTableHandle)) = malloc(sizeof(HashTableObject)); //allocates size of table
	(*(hashTableHandle))->sentinel = (int)0xDEADBEEF; //sets sentinel
	(*(hashTableHandle))->size = initialSize; //sets number of buckets
	(*(hashTableHandle))->elements = 0; //counter for number of keys in the table
	(*(hashTableHandle))->head_array = malloc(sizeof(bucket *) * initialSize);
	//note head_array is a void**, so all functions cast it to bucket**
	if((*(hashTableHandle))->head_array == NULL)
	{
		(*(hashTableHandle))->size = 0;
		return -1; //in case not enough memory is available (initialsize too large)
	}
	for (int i = 0; i < initialSize; i++)
	{ //head_array is an array of pointers to nodes
		((*(hashTableHandle))->head_array)[i] = NULL;//initializes head pointer to null
	}
	(*(hashTableHandle))->dynamic = 1; //initializes the dynamic parameters
	(*(hashTableHandle))->expand = (float)0.7;
	(*(hashTableHandle))->contract = (float)0.2;
	
	(*(hashTableHandle))->bucket_sizes = calloc(initialSize, sizeof(int));
	return 0;
}

int DestroyHashTable( HashTablePTR *hashTableHandle )
{
	if (*((int*)(*hashTableHandle)) != 0xDEADBEEF)
		return -1;
	if ((*hashTableHandle)->size == 0)
	{ //if nothing is in the table, just free the handle
		free(*hashTableHandle);
		*hashTableHandle = NULL;
		return -1;
	}
		
	//navigates through each linked list bucket and frees each node
	bucket *current; 
	for (int i = 0; i < (*hashTableHandle)->size; i++)
	{
		current = ((((bucket**)(*hashTableHandle)->head_array))[i]);
		FreeBinaryNode(current);
	}
	//after each bucket is emptied, frees the pointers in the hash table
	free((*hashTableHandle)->head_array);
	free((*hashTableHandle)->bucket_sizes);
	free(*hashTableHandle);
	*hashTableHandle = NULL;
	return 0;
}

int InsertEntry( HashTablePTR hashTable, char *key, void *data, void **previousDataHandle )
{
	if (hashTable == NULL)
		return -1;
	if (*((int*)(hashTable)) != 0xDEADBEEF)
		return -1;
		
	//makes copies of the key, not sure why
	char *key_copy = malloc(sizeof(char) * (strlen(key)+1));
	strcpy(key_copy, key);
	
	//hashes the key 
	int hash = (int)SimpleStringHash(key_copy, (unsigned char)hashTable->size);
	//the bucket to be deposited is the result of hashing
	bucket **current; 
	current = &(((bucket**)(hashTable->head_array))[hash]);
	
	if (*current == NULL)
	{ //if the bucket to be deposited in is empty 
		(((bucket**)(hashTable->head_array))[hash]) = NewNode(key_copy, data);
		(hashTable->elements) ++; //adds 1 to total number of entries in table
		(hashTable->bucket_sizes[hash]) ++; //adds 1 to the counter for the current bucket
		int i = checkRESIZE(hashTable);
		while (!i)
			i = checkRESIZE(hashTable);
		return 0;
	}
		
	while ((*current) != NULL)
	{
		if (!strcmp((*current)->key, key_copy)) //checks for key collisions
		{
			*previousDataHandle = ((*current)->data);
			(*current)->data = data;
			free(key_copy);
			return 2;
		}
		if (strcmp(key_copy, (*current)->key) < 0) //decide which child to insert into
			current = &((*current)->left);
		else 
			current = &((*current)->right);
	}
	
	//will reach here if only hash collision but key is unique
	(*current) = NewNode(key_copy, data);
	(hashTable->elements)++; //adds 1 to total number of entries in table
	(hashTable->bucket_sizes[hash])++;
	
	int i = checkRESIZE(hashTable);
	while (!i)
		i = checkRESIZE(hashTable);
	return 1;
}

static int DeleteNode (bucket** handle, char* value, void **dataHandle)
{
	if (*handle == NULL)
		return -2;
	else if (strcmp(value, (*handle)->key) < 0) // if value doesn't match, investigates children
		return DeleteNode(&((*handle)->left), value, dataHandle);
	else if (strcmp(value, (*handle)->key) > 0)
		return DeleteNode(&((*handle)->right), value, dataHandle);
		
	else if (!strcmp(value,(*handle)->key))
	{ // if matches, sets dataHandle, frees key
		*dataHandle = (*handle)->data;
		free((*handle)->key);
		if ((*handle)->left == NULL)
		{ // if either child is null, replace the deleted-node with the non-NULL one
			bucket *temp = *handle;
			*handle = (*handle)->right;
			free(temp);
		}
		else if ((*handle)->right == NULL)
		{
			bucket *temp = *handle;
			*handle = (*handle)->left;
			free(temp);
		}
		else
		{ // if both non-NULL, take the entire left subtree and shove under left-most leaf in right subtree
			bucket *temp = *handle;
			bucket *L = (*handle)->left;
			bucket *R = (*handle)->right;
			(*handle) = (*handle)->right;
			while (R->left != NULL)
				R = R->left; //navigates to find left-most leaf in right subtree
			R->left = L; //dumps left subtree at that location
			free(temp);
		}
	}
	return 0;
}

int DeleteEntry( HashTablePTR hashTable, char *key, void **dataHandle )
{
	if (hashTable == NULL)
		return -1;
	if (*((int*)(hashTable)) != 0xDEADBEEF)
		return -1;
		
	int hash = (int)SimpleStringHash(key, (unsigned char)hashTable->size);
	bucket **current;
	current = &(((bucket**)(hashTable->head_array))[hash]); //checks which bucket the key is in
	
	int success = DeleteNode(current, key, dataHandle);
	if (success == 0 && hashTable->elements > 0) //if a node is deleted, subtracts 1 from bucket_size
	{
		hashTable->bucket_sizes[hash] --;
		hashTable->elements --;
		int i = checkRESIZE(hashTable);
		while (!i && hashTable->elements > 0)
			i = checkRESIZE(hashTable);
	}
	
	return success;
}

int FindEntry( HashTablePTR hashTable, char *key, void **dataHandle )
{
	if (hashTable == NULL)
		return -1;
	if (*((int*)(hashTable)) != 0xDEADBEEF)
		return -1;
		
	int hash = (int)SimpleStringHash(key, (unsigned char)hashTable->size);
	bucket *current = (((bucket**)(hashTable->head_array))[hash]);
	
	while(current != NULL)
	{
		if (!strcmp(current->key, key))
		{ //hashes and checks whether the key is found, and sets the dataHandle if so
			*dataHandle = (current->data);
			return 0;
		}
		if (strcmp(key, current->key) < 0)
			current = current->left;
		else
			current = current->right;
	}
	return -2;
}
static void RetrieveKeys (char** keys_array, unsigned int *counter, bucket* current)
{ // function to add keys from hashTable to a string array
	if (current != NULL)
	{ //if the bucket pointer it receives is non-NULL, copies the key into array[index]
		keys_array[*counter] = malloc(sizeof(char) * (strlen(current->key) + 1));
		strcpy(keys_array[*counter], current->key);
		(*counter)++;
		//recursion on left/right subtree
		RetrieveKeys(keys_array, counter, current->left); 
		RetrieveKeys(keys_array, counter, current->right);
	}
}

int GetKeys( HashTablePTR hashTable, char ***keysArrayHandle, unsigned int *keyCount )
{
	if (hashTable == NULL)
		return -1;
	if (*((int*)(hashTable)) != 0xDEADBEEF)
		return -1;
		
	unsigned int key_index = 0; //counter for keys array
	// allocates an array of strings depending on number of elements in the hashtable
	*(keysArrayHandle) = malloc(sizeof(char*) * hashTable->elements);
	if(*keysArrayHandle == NULL)
		return -1;
	for (int i = 0; i < hashTable->size; i++)
	{ // for each bucket, use RetrieveKeys to get all the keys
		bucket *current = ((bucket**)(hashTable->head_array))[i];
		RetrieveKeys(*keysArrayHandle, &key_index, current);
	}
	*keyCount = key_index;
	return 0;
}

int GetHashTableInfo (HashTablePTR hashTable, HashTableInfo *pHashTableInfo)
{
	if (*((int*)(hashTable)) != 0xDEADBEEF)
		return -1;
	pHashTableInfo->bucketCount = hashTable->size;
	pHashTableInfo->loadFactor = ((float)hashTable->elements)/((float)hashTable->size);
	unsigned int non_empty = 0;
	for (int i = 0; i < hashTable->size; i++)
	{
		if ((hashTable->head_array)[i] != NULL)
			non_empty++;
	}
	pHashTableInfo->useFactor = (float)non_empty/(float)hashTable->size;
	
	unsigned int largest_bucket = 0;
	for (int i = 0; i < hashTable->size; i++)
	{
		if (hashTable->bucket_sizes[i] > largest_bucket)
			largest_bucket = hashTable->bucket_sizes[i];
	}
	
	pHashTableInfo->largestBucketSize = largest_bucket;
	pHashTableInfo->dynamicBehaviour = hashTable->dynamic;
	pHashTableInfo->expandUseFactor = hashTable->expand;
	pHashTableInfo->contractUseFactor = hashTable->contract;
	return 0;
}

int SetResizeBehaviour ( HashTablePTR hashTable, int dynamicBehaviour, float expandUseFactor, float contractUseFactor )
{
	if (*((int*)(hashTable)) != 0xDEADBEEF)
		return -1;
	if (expandUseFactor < contractUseFactor)
		return -1;
	hashTable->dynamic = dynamicBehaviour;
	hashTable->expand = expandUseFactor;
	hashTable->contract = contractUseFactor;
	return 0;
} 
