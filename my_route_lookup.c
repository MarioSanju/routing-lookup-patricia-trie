#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "io.h"
#include "utils.h"

// ESTRUCTURA DEL NODO PATRICIA

typedef struct Pnode {
    uint32_t prefix;      // prefijo asociado al nodo
    int plen;             // longitud del prefijo /X
    int bitIndex;         // bit asociado al nodo (en este diseño: plen o diffBit)

    int output;           // interfaz de salida
    bool valid;           // true = nodo real, false = nodo interseccion

    struct Pnode *left;
    struct Pnode *right;
} Pnode;

//FUNCIONES AUXILIARES DE BITS Y PREFIJOS

int getBit(uint32_t ip, int bitIndex) {
    if (bitIndex < 0 || bitIndex > 31) {
        fprintf(stderr, "Error: bitIndex fuera de rango (%d)\n", bitIndex);
        exit(EXIT_FAILURE);
    }

    return (ip >> (31 - bitIndex)) & 1;
}

int firstDiffBit(uint32_t p1, int len1, uint32_t p2, int len2) {
    int min = (len1 < len2) ? len1 : len2;

    for (int i = 0; i < min; i++) {
        if (getBit(p1, i) != getBit(p2, i)) {
            return i;
        }
    }

    return -1;
}

uint32_t buildMask(int plen) {
    if (plen < 0 || plen > 32) {
        fprintf(stderr, "Error: plen fuera de rango (%d)\n", plen);
        exit(EXIT_FAILURE);
    }

    if (plen == 0) return 0u;
    if (plen == 32) return 0xFFFFFFFFu;

    return 0xFFFFFFFFu << (32 - plen);
}

uint32_t normalizePrefix(uint32_t prefix, int plen) {
    return prefix & buildMask(plen);
}

bool matchPrefix(uint32_t ip, uint32_t prefix, int plen) {
    return normalizePrefix(ip, plen) == prefix;
}

int countNodes(Pnode *node) {
    if (node == NULL) {
        return 0;
    }

    return 1 + countNodes(node->left) + countNodes(node->right);
}

//CREACION DE NODOS

Pnode *createNode(uint32_t prefix, int plen, int bitIndex, int output, bool valid) {
    Pnode *node = (Pnode *)malloc(sizeof(Pnode));

    if (node == NULL) {
        fprintf(stderr, "Error: fallo del malloc al crear nodo\n");
        exit(EXIT_FAILURE);
    }

    node->prefix = normalizePrefix(prefix, plen);
    node->plen = plen;
    node->bitIndex = bitIndex;
    node->output = output;
    node->valid = valid;
    node->left = NULL;
    node->right = NULL;

    return node;
}


//INSERCION EN EL ARBOL
  

Pnode *insert(Pnode *node, uint32_t prefix, int plen, int output) {
    prefix = normalizePrefix(prefix, plen);

    /* Caso 1: hueco en el arbol -> crear hoja */
    if (node == NULL) {
        return createNode(prefix, plen, plen, output, true);
    }

    /* Caso 2: el prefijo ya existe -> actualizar */
    if ((node->prefix == prefix) && (node->plen == plen)) {
        node->output = output;
        node->valid = true;
        return node;
    }

    int diffBit = firstDiffBit(node->prefix, node->plen, prefix, plen);

    /* Caso 3: uno es prefijo del otro */
    if (diffBit == -1) {

        /* El nuevo es mas especifico -> baja como hijo */
        if (plen > node->plen) {
            if (node->plen == 32) {
                return node;
            }

            int bit = getBit(prefix, node->plen);

            if (bit == 0) {
                node->left = insert(node->left, prefix, plen, output);
            } else {
                node->right = insert(node->right, prefix, plen, output);
            }

            return node;
        }

        /* El nuevo es mas general -> pasa a ser padre */
        else {
            Pnode *oldNode = node;
            Pnode *newParent = createNode(prefix, plen, plen, output, true);

            int bit = getBit(oldNode->prefix, plen);

            if (bit == 0) {
                newParent->left = oldNode;
            } else {
                newParent->right = oldNode;
            }

            return newParent;
        }
    }

    /* Caso 4: divergen -> crear nodo de interseccion */
    else {
        uint32_t interPrefix = normalizePrefix(prefix, diffBit);

        Pnode *intersec = createNode(interPrefix, diffBit, diffBit, -1, false);
        Pnode *newNode = createNode(prefix, plen, plen, output, true);

        int bit = getBit(prefix, diffBit);

        if (bit == 0) {
            intersec->left = newNode;
            intersec->right = node;
        } else {
            intersec->right = newNode;
            intersec->left = node;
        }

        return intersec;
    }
}


Pnode *lookup(Pnode *node, uint32_t ip, Pnode *best, int *accesses) {
    if (node == NULL) {
        return best;
    }

    (*accesses)++;

    if (node->valid && matchPrefix(ip, node->prefix, node->plen)) {
        best = node;
    }

    if (node->bitIndex < 0 || node->bitIndex > 31) {
        return best;
    }

    int dir = getBit(ip, node->bitIndex);

    if (dir == 0) {
        return lookup(node->left, ip, best, accesses);
    } else {
        return lookup(node->right, ip, best, accesses);
    }
}

Pnode *lookupRoute(Pnode *root, uint32_t ip, int *accesses) {
    *accesses = 0;
    return lookup(root, ip, NULL, accesses);
}
//FUNCIONES DE DEPURACION / IMPRESION

void printIP(uint32_t ip) {
    printf("%u.%u.%u.%u",
           (ip >> 24) & 0xFF,
           (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF,
           ip & 0xFF);
}

void printNodeInfo(Pnode *node) {
    if (node == NULL) {
        printf("NULL");
        return;
    }

    printIP(node->prefix);
    printf("/%d", node->plen);
    printf(" [bitIndex=%d", node->bitIndex);
    printf(", valid=%s", node->valid ? "true" : "false");

    if (node->valid) {
        printf(", output=%d", node->output);
    } else {
        printf(", output=--");
    }

    printf("]");
}

void printTree(Pnode *node, int level, const char *label) {
    for (int i = 0; i < level; i++) {
        printf("    ");
    }

    printf("%s: ", label);

    if (node == NULL) {
        printf("NULL\n");
        return;
    }

    printNodeInfo(node);
    printf("\n");

    printTree(node->left, level + 1, "L");
    printTree(node->right, level + 1, "R");
}

//LIBERAR MEMORIA DEL ARBOL

void freeTree(Pnode *node) {
    if (node == NULL) {
        return;
    }

    freeTree(node->left);
    freeTree(node->right);
    free(node);
}
//pruebas lookup 

int main(int argc, char *argv[]) {
    int result;

    if (argc != 3) {
        printf("Usage: %s FIB InputPacketFile\n", argv[0]);
        return -1;
    }

    result = initializeIO(argv[1], argv[2]);
    if (result != OK) {
        printIOExplanationError(result);
        return -1;
    }

    Pnode *root = NULL;

    //LEER LA FIB 
    uint32_t prefix;
    int prefixLength;
    int outInterface;

    result = readFIBLine(&prefix, &prefixLength, &outInterface);
    while (result == OK) {
        root = insert(root, prefix, prefixLength, outInterface);
        result = readFIBLine(&prefix, &prefixLength, &outInterface);
    }

    if (result != REACHED_EOF) {
        printIOExplanationError(result);
        freeTree(root);
        freeIO();
        return -1;
    }

    //ESTADISTICAS
    int processedPackets = 0;
    long totalNodeAccesses = 0;
    double totalPacketProcessingTime = 0.0;
    int numberOfNodesInTrie = countNodes(root);

    //LEER IPS Y HACER LOOKUP 
    uint32_t ip;
    result = readInputPacketFileLine(&ip);

    while (result == OK) {
        struct timespec initialTime, finalTime;
        double searchingTime = 0.0;
        int numberOfAccesses = 0;

        clock_gettime(CLOCK_MONOTONIC_RAW, &initialTime);
        Pnode *best = lookupRoute(root, ip, &numberOfAccesses);
        clock_gettime(CLOCK_MONOTONIC_RAW, &finalTime);

        int outInterfaceResult = 0;
        if (best != NULL) {
            outInterfaceResult = best->output;
        }

        printOutputLine(ip, outInterfaceResult,
                        &initialTime, &finalTime,
                        &searchingTime, numberOfAccesses);

        processedPackets++;
        totalNodeAccesses += numberOfAccesses;
        totalPacketProcessingTime += searchingTime;

        result = readInputPacketFileLine(&ip);
    }

    if (result != REACHED_EOF) {
        printIOExplanationError(result);
        freeTree(root);
        freeIO();
        return -1;
    }

    double averageNodeAccesses = 0.0;
    double averagePacketProcessingTime = 0.0;

    if (processedPackets > 0) {
        averageNodeAccesses = (double) totalNodeAccesses / processedPackets;
        averagePacketProcessingTime = totalPacketProcessingTime / processedPackets;
    }

    printSummary(numberOfNodesInTrie, processedPackets,
                 averageNodeAccesses, averagePacketProcessingTime);

    freeTree(root);
    freeIO();
    return 0;
}

//Algoritmo de RL realizado por: Hugo Martinez (NIA:100522697) y Mario Sanjuan (NIA:100522935)
