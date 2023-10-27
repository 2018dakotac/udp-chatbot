#include "list.h"
#include <stdio.h>


/*
Written by Dakota Crozier 
CMPT 300
May 30th, 2021
UPDATED June 25th to fix concat pointer
*/

//initialize list
static List ListArr[LIST_MAX_NUM_HEADS];
static Node NodeArr[LIST_MAX_NUM_NODES];
static int ListInitialized = 0;
static Node End,Start;//empty nodes signifying before/beyond list this allows struct to be as small as possible

//decided to make a linked list of free nodes/heads within the arrays for maximum efficiency
static Node * freeNodePtr = NULL;
static List * freeHeadPtr = NULL;

//linked list functions
static void pushN(Node** head, Node *freeNode){
    freeNode->data = NULL;
    freeNode->next = NULL;
    freeNode->prev = NULL;
    freeNode->next = (*head);
    (*head) = freeNode;
}
static Node* removeN(Node**head){
    Node * tmp = (*head);
    if((*head) != NULL){
        (*head) = (*head)->next;
        tmp->next = NULL;
    }
    
    return tmp;
}
static void pushH(List**head,List *freeHead){
    freeHead->head = NULL;
    freeHead->tail = NULL;
    freeHead->current = NULL;
    freeHead->size = 0;
    freeHead->nextList = (*head);
    (*head) = freeHead;

}
static List* removeH(List**head){
    List * tmp = (*head);
    if((*head) != NULL){
        (*head) = (*head)->nextList;
        tmp->nextList = NULL;
    }
    
    return tmp;
}

//function to add a node to an empty list
static int List_empty_add(List* pList,void* pItem){
    //claim node
        Node * newNode = removeN(&freeNodePtr);
        if(newNode == NULL){
            return -1;
        }
        newNode->next = NULL;
        newNode->prev = NULL;
        newNode->data = pItem;//double check
        //update list
        pList->current = newNode;
        pList->head = newNode;
        pList->tail = newNode;
        pList->size++;
        return 0;
}

List* List_create(){
    //Initialize List and data structures if needed
    if(ListInitialized ==0){
  
    freeNodePtr = NULL;
    freeHeadPtr = NULL;
    //populate stack with free heads and nodes
    for(int i = 0;i<LIST_MAX_NUM_NODES;i++){
        pushN(&freeNodePtr,&NodeArr[i]);
    }
    for(int i = 0;i<LIST_MAX_NUM_HEADS;i++){
        pushH(&freeHeadPtr,&ListArr[i]);
    }    
    ListInitialized = 1;
    }
    //Try to create a list, get an available index
    //CLAIM A HEAD
    List * newHead = removeH(&freeHeadPtr);
    if(newHead == NULL){
        return newHead;
    }        
    //ListArr[freeIndex].index = freeIndex;
    newHead->current = NULL;
    newHead->tail = NULL;
    newHead->head = NULL;
    return newHead;
}


// Returns the number of items in pList.
int List_count(List* pList){
    return pList->size;
}
// Returns a pointer to the first item in pList and makes the first item the current item.
// Returns NULL and sets current item to NULL if list is empty.
void* List_first(List* pList){
    pList->current = pList->head;//if head NULL list empty therefore current item is NULL
	if(pList->current == NULL){
		return NULL;
	}
    return pList->current->data;
}

// Returns a pointer to the last item in pList and makes the last item the current item.
// Returns NULL and sets current item to NULL if list is empty.
void* List_last(List* pList){
    pList->current = pList->tail;
	if(pList->current == NULL){
		return NULL;
	}
    return pList->current->data;
}

// Advances pList's current item by one, and returns a pointer to the new current item.
// If this operation advances the current item beyond the end of the pList, a NULL pointer 
// is returned and the current item is set to be beyond end of pList.
void* List_next(List* pList){
    if(pList->head== NULL){//empty list check
        pList->current = &End;
        return NULL;
    }
    if(pList->current == &End){//already at end of list
        return NULL;
    }else if(pList->current == &Start){
        pList->current = pList->head;
        return pList->current->data;
    }
    pList->current = pList->current->next;
    if(pList->current!= NULL){
        return pList->current->data;
    }
    pList->current = &End;//must have been at tail if next is NULL 
    return NULL;
}

// Backs up pList's current item by one, and returns a pointer to the new current item. 
// If this operation backs up the current item beyond the start of the pList, a NULL pointer 
// is returned and the current item is set to be before the start of pList.
void* List_prev(List* pList){
    //assert(pList!=NULL);
    if(pList->head== NULL){//empty list check
        pList->current = &Start;
        return NULL;
    }
    if (pList->current==&Start){//before list
        return NULL;
    }else if(pList->current == &End){//beyond list
        pList->current = pList->tail;
        return pList->current->data;
    }
    pList->current = pList->current->prev;
    if(pList->current != NULL){
        return pList->current->data;
    }
    pList->current = &Start;//must have been at head if prev is NULL
    return NULL;

}

// Returns a pointer to the current item in pList.
void* List_curr(List* pList){
    if (pList->current==&Start|| pList->current == &End || pList->current == NULL){
        return NULL;
    }
    return pList->current->data;
}

// Adds the new item to pList directly after the current item, and makes item the current item. 
// If the current pointer is before the start of the pList, the item is added at the start. If 
// the current pointer is beyond the end of the pList, the item is added at the end. 
// Returns 0 on success, -1 on failure.
int List_add(List* pList, void* pItem){
    if(pList->head == NULL){//empty list
        return List_empty_add(pList,pItem);
    }
    //pointing before start of list
    if((*pList).current == &Start){
       return List_prepend(pList,pItem);

    } else if ((*pList).current == &End){//pointing after end of list
        return List_append(pList,pItem);
    }else {//regular add
        //claim a node
         Node * newNode = removeN(&freeNodePtr);
        if(newNode == NULL){
            return -1;
        }
        if((*pList).current->next != NULL){//check if there is a node after current
            newNode->next = (*pList).current->next;
            (*pList).current->next->prev = newNode;
        }else{
            newNode->next = NULL;
        }
        (*pList).current->next = newNode;
        newNode->prev = (*pList).current;
        newNode->data = pItem;
        if((*pList).tail == (*pList).current){
            (*pList).tail= newNode;
        }
        (*pList).current = newNode;
        pList->size++;
    }
    return 0;
}


// Adds item to pList directly before the current item, and makes the new item the current one. 
// If the current pointer is before the start of the pList, the item is added at the start. 
// If the current pointer is beyond the end of the pList, the item is added at the end. 
// Returns 0 on success, -1 on failure.
int List_insert(List* pList, void* pItem){
    if(pList->head == NULL){//empty list
        return List_empty_add(pList,pItem);
    }
    //pointing before start of list
    if((*pList).current == &Start){
       return List_prepend(pList,pItem);

    } else if ((*pList).current == &End){//pointing after end of list
        return List_append(pList,pItem);

    }else {//regular insert
        //claim a node
         Node * newNode = removeN(&freeNodePtr);
        if(newNode == NULL){
            return -1;
        }
        if(pList->current->prev != NULL){//check if there is a node before current
            newNode->prev = pList->current->prev;
            pList->current->prev->next = newNode;
        }else{
            newNode->prev = NULL;
        }
        pList->current->prev = newNode;
        newNode->next = pList->current;
        newNode->data = pItem;
        if(pList->current==pList->head){
            pList->head = newNode;
        }
        pList->current = newNode;
        pList->size++;
    }
    return 0;
}


// Adds item to the end of pList, and makes the new item the current one. 
// Returns 0 on success, -1 on failure.
int List_append(List* pList, void* pItem){
    if(pList->head == NULL){//empty list
        return List_empty_add(pList,pItem);
    }
    //claim a node
         Node * newNode = removeN(&freeNodePtr);
        if(newNode == NULL){
            return -1;
        }
        newNode->prev = pList->tail;
        pList->tail->next = newNode;
        pList->tail = newNode;
        pList->size++;
        pList->current = newNode;
        newNode->data = pItem;
        return 0;
}

// Adds item to the front of pList, and makes the new item the current one. 
// Returns 0 on success, -1 on failure.
int List_prepend(List* pList, void* pItem){
    if(pList->head == NULL){//empty list
        return List_empty_add(pList,pItem);
    }
       //claim node
        Node * newNode = removeN(&freeNodePtr);
        if(newNode == NULL){
            return -1;
        }
        newNode->next = pList->head;
        pList->head->prev = newNode;
        pList->head = newNode;
        pList->size++;
        pList->current = newNode;
        newNode->data = pItem;
         return 0;
}

// Return current item and take it out of pList. Make the next item the current one.
// If the current pointer is before the start of the pList, or beyond the end of the pList,
// then do not change the pList and return NULL.
void* List_remove(List* pList){
    if(pList->current == &Start || pList->current == &End||pList->current == NULL){//***assuming NULL current pointer means delete nothing and return NULL
        return NULL;
    }
    Node* tmp = pList->current;
    void* tmpData = tmp->data;
    if(pList->size == 1){//ez oneshot delete
        pList->current = NULL;
        pList->head = NULL;
        pList->tail = NULL;
        pushN(&freeNodePtr,tmp);
        pList->size--;
        return tmpData;
    }
    if(pList->current == pList->head){//deleting head
        tmp->next->prev = NULL;
        pList->head = tmp->next;
        pList->current = tmp->next;
    }else if(pList->current == pList->tail){//deleting tail
        tmp->prev->next = NULL;
        pList->tail = tmp->prev;
        pList->current = &End;//next item is end of list.
    }else{//middle of list
        pList->current = pList->current->next;
        tmp->prev->next = tmp->next;
        tmp->next->prev = tmp->prev;
    }
    pList->size--;
   pushN(&freeNodePtr,tmp);
    return tmpData;
}

// Adds pList2 to the end of pList1. The current pointer is set to the current pointer of pList1. 
// pList2 no longer exists after the operation; its head is available
// for future operations.
void List_concat(List* pList1, List* pList2){
    Node * originalCurr = pList1->current;
    List_first(pList2);
    List_last(pList1);
    while(pList2->current!=NULL){
        List_add(pList1,List_remove(pList2));
    }
    pushH(&freeHeadPtr,pList2);
    pList1->current = originalCurr;
}

// Delete pList. pItemFreeFn is a pointer to a routine that frees an item. 
// It should be invoked (within List_free) as: (*pItemFreeFn)(itemToBeFreedFromNode);
// pList and all its nodes no longer exists after the operation; its head and nodes are 
// available for future operations.
void List_free(List* pList, FREE_FN pItemFreeFn){
    if( pItemFreeFn == NULL){
        return;//WHY IS THIS A TEST CASE IF WE CAN ASSUME PLIST TO BE CORRECT 
    }
    List_first(pList);
    while(pList->current != NULL){
        (*pItemFreeFn)(List_remove(pList));
    }
    pushH(&freeHeadPtr,pList);
}

// Return last item and take it out of pList. Make the new last item the current one.
// Return NULL if pList is initially empty.
void* List_trim(List* pList){
    if(pList->head == NULL){
        return NULL;
    }
    Node* tmp = pList->tail;
    void* tmpData = tmp->data;
    if(pList->size == 1){//ez oneshot delete
        pList->current = NULL;
        pList->head = NULL;
        pList->tail = NULL;
        pushN(&freeNodePtr,tmp);
        pList->size--;
        return tmpData;
    }
    pList->tail = tmp->prev;
    tmp->prev->next = NULL;
    pList->current = pList->tail;
    
    pushN(&freeNodePtr,tmp);
    pList->size--;
    return tmpData;
    
}

// Search pList, starting at the current item, until the end is reached or a match is found. 
// In this context, a match is determined by the comparator parameter. This parameter is a
// pointer to a routine that takes as its first argument an item pointer, and as its second 
// argument pComparisonArg. Comparator returns 0 if the item and comparisonArg don't match, 
// or 1 if they do. Exactly what constitutes a match is up to the implementor of comparator. 
// 
// If a match is found, the current pointer is left at the matched item and the pointer to 
// that item is returned. If no match is found, the current pointer is left beyond the end of 
// the list and a NULL pointer is returned.
// 
// If the current pointer is before the start of the pList, then start searching from
// the first node in the list (if any).
//typedef bool (*COMPARATOR_FN)(void* pItem, void* pComparisonArg);
void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg){
    if( pComparator == NULL){
        return NULL;//WHY IS THIS A TEST CASE IF WE CAN ASSUME PLIST TO BE CORRECT 
    }
    if(pList->current == &Start){
        pList->current = pList->head;
    }
    if(pList->current == NULL||pList->current==&End){//empty or at end of list therefore no search
        return NULL;
    }
    while(pList->current != NULL){
        if((*pComparator)(pList->current->data,pComparisonArg)){
            return pList->current->data;
        }
        pList->current = pList->current->next;
    }
    pList->current = &End;
    return NULL;

}