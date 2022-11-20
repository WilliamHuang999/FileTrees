/*--------------------------------------------------------------------*/
/* nodef.c                                                            */
/* Author: Will Huang and George Tziampazis                           */
/*--------------------------------------------------------------------*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "nodef.h"

/* A file node in a FT */
struct nodeF {
   /* Object corresponding to the node's absolute path */
   Path_T oPPath;

   /* Size of file contents in bytes */
   size_t ulLength;

   /* Contents of the file */
   void *pvContents;
};

/* ================================================================== */
int NodeF_new(Path_T oPPath, NodeF_T *poNfResult) {
   NodeF_T oNfNew;   /* New file node to be created */
   Path_T oPNewPath; /* New path of new file node */
   int iStatus;

   assert(oPPath != NULL);

   /* Path of file cannot be 0 or 1 */
   if (!(Path_getDepth(oPPath) > 1)) {
      *poNfResult = NULL;
      return NO_SUCH_PATH;
   }

   /* Allocate mem for new node and check for enough mem */
   oNfNew = (oNfNew)malloc(sizeof(struct nodeF));
   if(oNfNew == NULL) {
      *poNfResult = NULL;
      return MEMORY_ERROR;
   }

   /* Set the new node's path and check for enough mem */
   iStatus = Path_dup(oPPath, &oPNewPath);
   if(iStatus != SUCCESS) {
      free(oNfNew);
      *poNfResult = NULL;
      return iStatus;
   }
   oNfNew->oPPath = oPNewPath;

   /* Set initial values of file contents and size*/
   oNfNew->ulLength = 0;
   oNfNew->pvContents = NULL;

   *poNfResult = oNfNew;

   return SUCCESS;
}

/* ================================================================== */
size_t NodeF_free(NodeF_T oNfNode) {
   size_t ulSize;

   assert(oNfNode != NULL);

   ulSize = oNfNode->ulLength;

   /* Remove path */
   Path_free(oNfNode->oPPath);
   /* Free the actual file node */
   free(oNfNode);

   return ulSize;
}

/* ================================================================== */
Path_T NodeF_getPath(NodeF_T oNfNode) {
   assert(oNfNode != NULL);
   return oNfNode->oPPath;
}

/* ================================================================== */
int NodeF_compare(NodeF_T oNfNode1, NodeF_T oNfNode2) {
   assert(oNfNode1 != NULL);
   assert(oNfNode2 != NULL);

   return Path_comparePath(oNfNode1->oPPath, oNfNode2->oPPath);
}

/* ================================================================== */
char *NodeF_toString(NodeF_T oNfNode) {
   char *pcResult;   /* String representation of oNFNode */

   assert(oNfNode != NULL);

   /* Allocate mem for pcResult and check if enough mem */
   pcResult = malloc(Path_getStrLength(Node_getPath(oNfNode))+1);
   if(pcResult == NULL) {
      return NULL;
   }
   /* Copy path name to pcResult and return the copy */
   return strcpy(pcResult, Path_getPathname(Node_getPath(oNfNode)));
}