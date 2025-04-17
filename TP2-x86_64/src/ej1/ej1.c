#include "ej1.h"

/* 1) Crear una lista vacía */
string_proc_list* string_proc_list_create(void) {
    string_proc_list* list = malloc(sizeof *list);
    if (list == NULL) {
        return NULL;
    }
    list->first = NULL;
    list->last  = NULL;
    return list;
}

/* 2) Crear un nodo apuntando al hash dado */
string_proc_node* string_proc_node_create(uint8_t type, char* hash) {
    if (hash == NULL) {
        return NULL;
    }
    string_proc_node* node = malloc(sizeof *node);
    if (node == NULL) {
        return NULL;
    }
    node->next     = NULL;
    node->previous = NULL;
    node->hash     = hash;
    node->type     = type;
    return node;
}

/* 3) Añadir un nodo al final de la lista */
void string_proc_list_add_node(string_proc_list* list,
                               uint8_t           type,
                               char*             hash)
{
    if (list == NULL) {
        return;
    }
    string_proc_node* node = string_proc_node_create(type, hash);
    if (node == NULL) {
        return;
    }
    if (list->first == NULL) {
        list->first = node;
        list->last  = node;
    } else {
        list->last->next   = node;
        node->previous     = list->last;
        list->last         = node;
    }
}

/* 4) Concatenar todos los hashes de nodos de un tipo dado */
char* string_proc_list_concat(string_proc_list* list,
                              uint8_t           type,
                              char*             base_hash)
{

    if (list == NULL || base_hash == NULL) {
        return NULL;
    }

    string_proc_node* cur    = list->first;
    char*              result = NULL;

    while (cur) {
        if (cur->type == type) {
            size_t part_len = strlen(cur->hash);

            if (result == NULL) {
                /* primer match: base_hash + cur->hash */
                size_t base_len = strlen(base_hash);
                /* evita overflow */
                if (base_len > SIZE_MAX - part_len - 1) {
                    return NULL;
                }
                result = malloc(base_len + part_len + 1);
                if (result == NULL) {
                    return NULL;
                }
                memcpy(result,             base_hash, base_len);
                memcpy(result + base_len,  cur->hash,  part_len);
                result[base_len + part_len] = '\0';
            } else {
                /* match subsiguiente: realloc-like */
                size_t prev_len = strlen(result);
                if (prev_len > SIZE_MAX - part_len - 1) {
                    free(result);
                    return NULL;
                }
                char* tmp = malloc(prev_len + part_len + 1);
                if (tmp == NULL) {
                    free(result);
                    return NULL;
                }
                memcpy(tmp,               result,    prev_len);
                memcpy(tmp + prev_len,   cur->hash, part_len);
                tmp[prev_len + part_len] = '\0';
                free(result);
                result = tmp;
            }
        }
        cur = cur->next;
    }

    if (result == NULL) {
        /* ningún match: devolvemos cadena vacía */
        result = malloc(1);
        if (result) {
            result[0] = '\0';
        }
    }

    return result;
}

/** AUX FUNCTIONS **/

void string_proc_list_destroy(string_proc_list* list){

	/* borro los nodos: */
	string_proc_node* current_node	= list->first;
	string_proc_node* next_node		= NULL;
	while(current_node != NULL){
		next_node = current_node->next;
		string_proc_node_destroy(current_node);
		current_node	= next_node;
	}
	/*borro la lista:*/
	list->first = NULL;
	list->last  = NULL;
	free(list);
}
void string_proc_node_destroy(string_proc_node* node){
	node->next      = NULL;
	node->previous	= NULL;
	node->hash		= NULL;
	node->type      = 0;			
	free(node);
}


char* str_concat(char* a, char* b) {
	int len1 = strlen(a);
    int len2 = strlen(b);
	int totalLength = len1 + len2;
    char *result = (char *)malloc(totalLength + 1); 
    strcpy(result, a);
    strcat(result, b);
    return result;  
}

void string_proc_list_print(string_proc_list* list, FILE* file){
        uint32_t length = 0;
        string_proc_node* current_node  = list->first;
        while(current_node != NULL){
                length++;
                current_node = current_node->next;
        }
        fprintf( file, "List length: %d\n", length );
		current_node    = list->first;
        while(current_node != NULL){
                fprintf(file, "\tnode hash: %s | type: %d\n", current_node->hash, current_node->type);
                current_node = current_node->next;
        }
}

