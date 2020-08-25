/**
 * list.c for Assignment 1, CMPT 300 Summer 2020
 * Name: Devansh Chopra
 * Student #: 301-275-491
 */

// Header files imported
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Declaring List Array and Nodes Array of structs
static List Arr[LIST_MAX_NUM_HEADS];
static Node Nodes[LIST_MAX_NUM_NODES];

// Helper variables
static int Head_Index= 0;
static int Head_List_counter = 0;
static List* HeadsList = NULL;
static int Node_Index = 0;
static int Node_List_counter = 0;
static Node* NodesList = NULL;
static bool FirstTime_ListCreate = true;

// Helper Functions 
static void* HeadList_Pop();
static void HeadList_Push(void* pItem);
static void* NodesList_Pop();
static void NodesList_Push(void* pItem);

// Creates an empty list
List* List_create()
{
    // If List_create called for the first time, go in the below if statement
    if (FirstTime_ListCreate){
        for(int i=0; i <= LIST_MAX_NUM_HEADS-1; i++){
            Arr[i].head = NULL;
            Arr[i].current = NULL;
            Arr[i].tail = NULL;
            Arr[i].check_if_at_head = false;
            Arr[i].check_if_at_tail = false;
            Arr[i].number_of_nodes = 0;
        }

        for(int j=0; j <= LIST_MAX_NUM_NODES-1; j++){
            Nodes[j].value = 0;
            Nodes[j].next = NULL;
            Nodes[j].previous = NULL;
        }
        FirstTime_ListCreate = false;
    }
    
    // Creating a new list
    List* NewList;
    
    // If array of Heads is not fully used, use the availbe heads from the array
    if (Head_Index < LIST_MAX_NUM_HEADS){
        NewList = &(Arr[Head_Index]);
        Head_Index++;
    }

    // If array is fully used, find a free head from the self-created stack
    else{
        // If nothing is free, return NULL
        if(Head_List_counter == 0){
            return NULL;
        }
        NewList = HeadList_Pop();
    }
    return NewList;
}

// Returns the number of items in pList.
int List_count(List* pList)
{
    // If a bad List is passed in return -1
    if(pList==NULL){
        return -1;
    }
    // If a non-existent list is passed, return -1
    if(pList->number_of_nodes < 0){
        return -1;
    }
    // Return number of nodes in a list
    else{
        return pList->number_of_nodes;
    }
}

// Returns a pointer to the first item in pList and makes the first item the current item.
void* List_first(List* pList)
{
    // If a bad List is passed in or a non-existent list, return NULL pointer
    if(pList->number_of_nodes < 0 || pList == NULL){
        return NULL;
    }
    // If the list is empty then return NULL pointer
    if(pList->head == NULL){
        pList->current = NULL;
        return NULL;
    }
    // If current pointer is before the start of the list/beyond the end of the list, bring them back
    if(pList->check_if_at_head || pList->check_if_at_tail){
        pList->check_if_at_head = false;
        pList->check_if_at_tail = false;
    }
    // Set the head to the current pointer
    pList->current = pList->head;
    Node* current_pointer = pList->current;
    return current_pointer->value;
}

// Returns a pointer to the last item in pList and makes the last item the current item.
void* List_last(List* pList)
{
    // If a bad List is passed in or a non-existent list, return NULL pointer
    if(pList->number_of_nodes < 0 || pList == NULL){
        return NULL;
    }
    // If the list is empty then return NULL pointer
    if(pList->head == NULL){
        pList->current = NULL;
        return NULL;
    }
    // If current pointer is before the start of the list/beyond the end of the list, bring them back
    if(pList->check_if_at_head || pList->check_if_at_tail){
        pList->check_if_at_head = false;
        pList->check_if_at_tail = false;
    }
    // Set the tail to the current pointer
    pList->current = pList->tail;
    Node* current_pointer = pList->current;
    return current_pointer->value;
}

// Advances pList's current item by one, and returns a pointer to the new current item.
void* List_next(List* pList)
{
    // If a bad List is passed in or a non-existent list, return NULL pointer
    if(pList->number_of_nodes < 0 || pList == NULL){
        return NULL;
    }
    // If the list is empty then return NULL pointer
    if(pList->head == NULL){
        pList->current = NULL;
        return NULL;
    }
    // If the current pointer is before the start of the list, bring it to the first item
    else if(pList->check_if_at_head){
        pList->current = pList->head;
        Node* current_pointer = pList->current;
        pList->check_if_at_head = false;
        return current_pointer->value;
    }
    // If the current pointer is at the tail, make it go beyond the end of the list and return a NULL pointer
    else if (pList->current == pList->tail){
        pList->check_if_at_tail = true;
        return NULL;
    }
    // If the current pointer is beyond the end of the list, stay there and return a NULL pointer
    else if (pList->check_if_at_tail){
        return NULL;
    }
    // Normal case 
    else {
        Node* current_pointer = pList->current;
        pList->current = current_pointer->next;
        current_pointer = pList->current;
        return current_pointer->value;
    }
}

// Backs up pList's current item by one, and returns a pointer to the new current item. 
void* List_prev(List* pList)
{
    // If a bad List is passed in or a non-existent list, return NULL pointer
    if(pList->number_of_nodes < 0 || pList == NULL){
        return NULL;
    }
    // If the list is empty then return NULL pointer
    if(pList->head == NULL){
        pList->current = NULL;
        return NULL;
    }
    // If the current pointer is at the head, make it go before the start of the list and return a NULL pointer
    else if (pList->current == pList->head){
        pList->check_if_at_head = true;
        return NULL;
    }
    // If the current pointer is before the start of the list, stay there and return a NULL pointer
    else if(pList->check_if_at_head){
        return NULL;
    }
    // If the current pointer is beyond the end of the list, bring it back to the tail
    else if(pList->check_if_at_tail){
        pList->current = pList->tail;
        Node* current_pointer = pList->current;
        pList->check_if_at_tail = false;
        return current_pointer->value;
    }
    // Normal case
    else{
        Node* current_pointer = pList->current;
        pList->current = current_pointer->previous;
        current_pointer = pList->current;
        return current_pointer->value;
    }
}

// Returns a pointer to the current item in pList.
void* List_curr(List* pList)
{
    // If a bad List is passed in or a non-existent list, return NULL pointer
    if(pList->number_of_nodes < 0 || pList == NULL){
        return NULL;
    }
    // If the list is empty/beyond the end/before the start then return NULL pointer
    if (pList->check_if_at_head || pList->check_if_at_tail || (pList->head == NULL && pList->head == pList->current)){
        return NULL;
    }
    // Normal case
    else{
        Node* current_pointer = pList->current;
        return current_pointer->value;
    }
}

// Adds the new item to pList directly after the current item, and makes item the current item. 
int List_add(List* pList, void* pItem)
{
    // If a bad List is passed in or a non-existent list, return -1
    if(pList->number_of_nodes < 0 || pList == NULL){
        return -1;
    }
    Node* new;
    // Use any free nodes from the stack
    if (Node_Index >= LIST_MAX_NUM_NODES){
        // If all nodes are used up, return -1
        if(Node_List_counter == 0){
            return -1;
        }
        new = NodesList_Pop();
    }
    // If array still has free nodes, use from array
    else{
        new = &(Nodes[Node_Index]);
        Node_Index++;
    }
    new->value = pItem;
    // If list is empty, add to the list 
    if(pList->head == NULL){
        pList->head = new;
        pList->tail = new;
        pList->current = new;
    }
    // If current item is beyond the end of the list, add at the tail
    else if(pList->check_if_at_tail && pList->tail != NULL){
        Node* temp = pList->tail;
        temp->next = new;
        pList->check_if_at_tail = false;
        new->previous = pList->tail;
        pList->tail = new;
        pList->current = new;
    }
    // If current item is before the start of the list, add at the head
    else if(pList->check_if_at_head && pList->head != NULL){
        Node* temp = pList->head;
        pList->check_if_at_head = false;
        temp->previous = new;
        new->next = pList->head;
        pList->head = new;
        pList->current = new;
    }
    // Normal case
    else{
        Node* temp = pList->current;
        new->next = temp->next;
        new->previous = pList->current;
        temp = pList->current;
        temp->next = new;

        if(pList->current == pList->tail){
            pList->tail = new;
        }
        else{
        temp = new->next;
        temp->previous = new;
        }
        pList->current = new;
    }
    pList->number_of_nodes++;
    return 0;
}

// Adds item to pList directly before the current item, and makes the new item the current one. 
int List_insert(List* pList, void* pItem)
{
    // If a bad List is passed in or a non-existent list, return -1
    if(pList->number_of_nodes < 0 || pList == NULL){
        return -1;
    }
    Node* new;
    // Use any free nodes from the stack
    if (Node_Index >= LIST_MAX_NUM_NODES){
        // If all nodes are used up, return -1
        if(Node_List_counter == 0){
            return -1;
        }
        new = NodesList_Pop();
    }
    // If array still has free nodes, use from array
    else{
        new = &(Nodes[Node_Index]);
        Node_Index++;
    }
    new->value = pItem;
    // If list is empty, add to the list 
    if(pList->head == NULL){
        pList->head = new;
        pList->tail = new;
        pList->current = new;
    }
    // If current item is before the start of the list, add at the head 
    else if(pList->check_if_at_head  && pList->head != NULL){
        Node* temp = pList->head;
        pList->check_if_at_head = false;
        temp->previous = new;
        new->next = pList->head;
        pList->current = new;
        pList->head = new;
    }
    // If current item is beyond the end of the list, add at the tail
    else if(pList->check_if_at_tail && pList->tail != NULL){
        Node* temp = pList->tail;
        temp->next = new;
        pList->check_if_at_tail = false;
        new->previous = pList->tail;
        pList->tail = new;
        pList->current = new;
    }
    // Normal case
    else{
        Node* temp = pList->current;
        new->previous = temp->previous;
        new->next = pList->current;
        temp = pList->current;
        temp->previous = new;

        if(pList->current == pList->head){
            pList->head = new;
        }
        else{
            temp = new->previous;
            temp->next = new;
        }
        pList->current = new;
    }
    pList->number_of_nodes++;
    return 0;
}

// Adds item to the end of pList, and makes the new item the current one. 
int List_append(List* pList, void* pItem)
{
    // If a bad List is passed in or a non-existent list, return -1
    if(pList->number_of_nodes < 0 || pList == NULL){
        return -1;
    }
    Node* new;
    // Use any free nodes from the stack
    if (Node_Index >= LIST_MAX_NUM_NODES){
        if(Node_List_counter == 0){
            return -1;
        }
        new = NodesList_Pop();
    }
    // If array still has free nodes, use from array
    else{
        new = &(Nodes[Node_Index]);
        Node_Index++;
    }
    new->value = pItem;
    // If list is empty, add to the list 
    if(pList->head == NULL){
        pList->head = new;
        pList->tail = new;
        pList->current = new;
    }
    // Normal case
    else{
        new->previous = pList->tail;
        Node* temp = pList->tail;
        temp->next = new;
        pList->tail = new;
        pList->current = new;
    }
    pList->number_of_nodes++;
    return 0;
}

// Adds item to the front of pList, and makes the new item the current one. 
int List_prepend(List* pList, void* pItem)
{
    // If a bad List is passed in or a non-existent list, return -1
    if(pList->number_of_nodes < 0 || pList == NULL){
        return -1;
    }
    Node* new;
    // Use any free nodes from the stack
    if (Node_Index >= LIST_MAX_NUM_NODES){
        // If all nodes are used up, return -1
        if(Node_List_counter == 0){
            return -1;
        }
        new = NodesList_Pop();
    }
    // If array still has free nodes, use from array
    else{
        new = &(Nodes[Node_Index]);
        Node_Index++;
    }
    new->value = pItem;
    // If list is empty, add to the list 
    if(pList->head == NULL){
        pList->head = new;
        pList->tail = new;
        pList->current = new;
    }
    // Normal case
    else{
        new->next = pList->head;
        Node* temp = pList->head;
        temp->previous = new;
        pList->head = new;
        pList->current = new;
    }
    pList->number_of_nodes++;
    return 0;
}

// Return current item and take it out of pList. Make the next item the current one.
void* List_remove(List* pList)
{
    // If a bad List is passed in or a non-existent list, return -1
    if(pList->number_of_nodes < 0 || pList == NULL){
        return NULL;
    }
    // If current item is before/beyond the list or list is empty, return NULL pointer 
    if (pList->check_if_at_head || pList->check_if_at_tail || (pList->head == NULL && pList->head == pList->current)){
        return NULL;
    }
    // Node to remove
    Node* temp = pList->current;
    // If there is only one node in the plist
    if(pList->number_of_nodes == 1){
        pList->head = NULL;
        pList->tail = NULL;
        pList->current = NULL;
    }
    // Removing at the head
    else if (pList->current == pList->head && pList->current != NULL && pList->head != NULL) {
        pList->head = temp->next;
        Node* temp1 = pList->head;
        temp1->previous = NULL;
        pList->current = pList->head;
    }
    // Removing at the tail
    else if (pList->current == pList->tail && pList->current != NULL && pList->tail != NULL) {
        pList->tail = temp->previous;
        Node* temp1 = pList->tail;
        temp1->next = NULL;
        pList->check_if_at_tail = true;
        pList->current = pList->tail;
    }
    // Normal case
    else {
        Node* temp = pList->current;
        Node* temp1 = temp->previous;
        temp1->next = temp->next;
        Node* temp2 = temp->next;
        temp2->previous = temp->previous;
        pList->current = temp2;
    }
    // Push the removed node on the stack to use later
    NodesList_Push(temp);
    pList->number_of_nodes--;
    return temp->value;  
}

// Adds pList2 to the end of pList1. The current pointer is set to the current pointer of pList1. 
void List_concat(List* pList1, List* pList2)
{
    // If a bad List is passed in or a non-existent list, return
    if(pList1->number_of_nodes < 0 || pList1 == NULL || pList2->number_of_nodes < 0 || pList2 == NULL){
        return;
    }
    // If both lists are the same, return
    if(pList1 == pList2){
        return;
    }
    // If list1 is empty and list2 is empty
    if(pList1->head == NULL && pList2->head == NULL){

    }
    // If list1 is empty and list2 is not empty
    else if(pList1->head == NULL && pList2->head != NULL){
        pList1->head = pList2->head;
        pList1->tail = pList2->tail;
    }
    // If list1 is not empty and list 2 is empty 
    else if(pList1->head != NULL && pList2->head == NULL){

    }
    // Normal case
    else{
        Node* temp = pList1->tail;
        temp->next = pList2->head;
        temp = pList2->head;
        temp->previous = pList1->tail;
        pList1->tail = pList2->tail;
    }

    pList1->number_of_nodes += pList2->number_of_nodes;
    pList2->head = NULL;
    pList2->current = NULL;
    pList2->tail = NULL;
    pList2->check_if_at_head = false;
    pList2->check_if_at_tail = false;
    pList2->number_of_nodes = -1;
    HeadList_Push(pList2);
}

// Delete pList. itemFree is a pointer to a routine that frees an item. 
void List_free(List* pList, FREE_FN pItemFreeFn)
{
    // If a bad List is passed in or a non-existent list, return
    if(pList == NULL || pList->number_of_nodes < 0){
        return;
    }
    // If list is empty 
    if(pList->head == NULL && pList->current == NULL && pList->tail == NULL && pList->number_of_nodes == 0){
        pList->head = NULL;
        pList->current = NULL;
        pList->tail = NULL;
        pList->check_if_at_head = false;
        pList->check_if_at_tail = false;
        pList->number_of_nodes = -1;
        HeadList_Push(pList);
        return;
    }
    pList->check_if_at_head = false;
    pList->check_if_at_tail = false;
    pList->current = pList->head;
    Node* temp;
    // List is not empty
    while(pList->current){
        temp = pList->current;
        if(pList->current == pList->tail){
            (*pItemFreeFn)(temp->value);
            break;
        }
        (*pItemFreeFn)(temp->value);
        List_remove(pList);
    }
    temp = pList->current;
    temp->next = NULL;
    temp->previous = NULL;
    temp->value = 0;
    NodesList_Push(temp);

    pList->head = NULL;
    pList->current = NULL;
    pList->tail = NULL;
    pList->number_of_nodes = -1;
    pList->check_if_at_head = false;
    pList->check_if_at_tail = false;
    HeadList_Push(pList); 
}

// Return last item and take it out of pList. Make the new last item the current one.
void* List_trim(List* pList)
{
    // If a bad List is passed in or a non-existent list, return NULL pointer
    if(pList->number_of_nodes < 0 || pList == NULL){
        return NULL;
    }
    // If a list is empty, return NULL pointer
    if(pList->head == NULL && pList->tail == NULL && pList->number_of_nodes == 0){
        return NULL;
    }
    pList->current = pList->tail;
    Node* temp = List_remove(pList);
    pList->check_if_at_head = false;
    pList->check_if_at_tail = false;
    return temp;
}

// If a match is found, the current pointer is left at the matched item and the pointer to 
// that item is returned. If no match is found, the current pointer is left beyond the end of 
// the list and a NULL pointer is returned.
void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg)
{
    // If a bad List is passed in or a non-existent list, return pointer
    if(pList->number_of_nodes < 0 || pList == NULL){
        return NULL;
    }
    // If a list is empty, return a NULL pointer
    if(pList->head == NULL){
        return NULL;
    }
    // Find the item
    Node* temp;
    while(pList->current){
        temp = pList->current;
        if(temp->value && (*pComparator)(temp->value, pComparisonArg)){
            pList->current = temp;
            return temp->value;
        }
        pList->current = temp->next;

    }
    pList->check_if_at_tail = true;
    return NULL;
}

// Node that gets removed, goes back to the pool of Nodes
static void NodesList_Push(void* pItem){
    Node* temp = pItem;
    if(NodesList == NULL){
        temp->next = NULL;
        temp->previous = NULL;
        NodesList = temp;
    }
    else{
        temp->next = NodesList;
        temp->previous = NULL;
        NodesList = temp;
    }
    Node_List_counter++;
    return;
}

// Gets an avaible node from the pool of nodes
static void* NodesList_Pop(){
    if(NodesList != NULL){
        Node* temp = NodesList;
        NodesList = temp->next;
        temp->next = NULL;
        temp->previous = NULL;
        temp->value = 0;
        Node_List_counter--;
        return temp;
    }
    else{
     return NULL;
    }
}

// Heads that gets freed, go back to the pool of Heads
static void HeadList_Push(void* pItem){
    List* temp = pItem;
    if(HeadsList == NULL){
        temp->next = NULL;
        HeadsList = temp;
    }
    else{
        temp->next = HeadsList;
        HeadsList = temp;
    }
    Head_List_counter++;
    return;
}

// Gets an avaible head from the pool of Heads
static void* HeadList_Pop(){
    if(HeadsList != NULL){
        List* temp = HeadsList;
        HeadsList = temp->next;
        temp->current = NULL;
        temp->head = NULL;
        temp->tail = NULL;
        temp->check_if_at_head = false;
        temp->check_if_at_tail = false;
        temp->number_of_nodes = 0;
        Head_List_counter--;
        return temp;
    }
    else{
        return NULL;
    }
}
