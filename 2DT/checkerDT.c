/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/* Author: Will Huang and George Tziampazis                           */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"

/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
   Node_T oNParent;  /* Parent of oNNode */
   Node_T oNChild1;  /* Child of oNNode */
   Node_T oNChild2;  /* Another child of oNNode */
   Path_T oPNPath;   /* Path of oNNode */
   Path_T oPPPath;   /* Path of parent node */
   Path_T oPChildPath1; /* Path of oNChild1 */
   Path_T oPChildPath2; /* Path of oNChild2 */
   int iStatus;
   /* Indices for looping thru children of oNNode */
   size_t i;
   size_t j;

   /* Sample check: a NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }

   /* Sample check: parent's path must be the longest possible
      proper prefix of the node's path */
   oNParent = Node_getParent(oNNode);
   if(oNParent != NULL) {
      oPNPath = Node_getPath(oNNode);
      oPPPath = Node_getPath(oNParent);

      if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
      Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }
   }

   oPNPath = Node_getPath(oNNode);
   /* Iterate thru children of oNNode */
   for(i = 0; i < Node_getNumChildren(oNNode); i++) {
      oNChild1 = NULL;
      iStatus = Node_getChild(oNNode, i, &oNChild1);
      if(iStatus != SUCCESS) {
         fprintf(stderr, "getNumChildren claims more children than \
getChild returns\n");
         return FALSE;
      }
      oPChildPath1 = Node_getPath(oNChild1);
      

      /* dtBad2 invariant: two nodes cannot have same absolute path */
      if (Path_comparePath(oPNPath, oPChildPath1) == 0) {
         fprintf(stderr,
         "Two nodes cannot have the same absolute path. Parent: (%s). \
Child: (%s)\n",
         Path_getPathname(oPNPath),Path_getPathname(oPChildPath1));
         return FALSE;
      }

      /* Iterate thru children of oNNode starting from index i */
      for (j = i + 1; j < Node_getNumChildren(oNNode); j++) {
         oNChild2 = NULL;
         iStatus = Node_getChild(oNNode,j,&oNChild2);
         if (iStatus != SUCCESS) {
            fprintf(stderr, "getNumChildren claims more children than \
getChild returns\n");
            return FALSE;
         }
         oPChildPath2 = Node_getPath(oNChild2);
         
         /*dtBad2 invariant: two nodes cannot have same absolute path*/
         if (Path_comparePath(oPChildPath1,oPChildPath2) == 0) {
            fprintf(stderr, "Two nodes cannot have same absolute path. \
Child: (%s). Child: (%s)\n",
            Path_getPathname(oPChildPath1),
            Path_getPathname(oPChildPath2));
            return FALSE;
         }

         /* dtBad3 invariant: children must be in lexicographic order */
         if (!(Path_comparePath(oPChildPath1,oPChildPath2) < 0)) {
            fprintf(stderr, "Children are not in lexicographic order. \
Child: (%s). Child: (%s)\n",
            Path_getPathname(oPChildPath1),
            Path_getPathname(oPChildPath2));
            return FALSE;
         }
      }
   }
   return TRUE; 
}


/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Takes size_t nodeCount representing the "position" of the current
   node. Increments count to go to the next node recursively.
   Returns current node count or 0 (FALSE) if broken invariant is found.
*/
static size_t CheckerDT_treeCheck(Node_T oNNode,size_t nodeCount) {
   size_t ulIndex;

   if(oNNode!= NULL) {
      nodeCount++;    /* Increment the count of number of nodes */

      /* Sample check on each node: node must be valid */
      /* If not, pass that failure back up immediately */
      if(!CheckerDT_Node_isValid(oNNode))
         return (size_t)0;

      /* Recur on every child of oNNode */
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++)
      {
         Node_T oNChild = NULL;
         int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);

         if(iStatus != SUCCESS) {
            fprintf(stderr,"getNumChildren claims more children than \
            getChild returns\n");
            return (size_t)0;
         }

         /* Recur down subtree and keep track of number of nodes
         recurred. */
         nodeCount = CheckerDT_treeCheck(oNChild,nodeCount);
         /* If nodeCount is 0, a broken invariant was found in a
          subtree*/
         if (!nodeCount) {
            return (size_t)0;
         }
      }
   }
   return nodeCount;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {
   size_t nodeCount;

   /* Sample check on a top-level data structure invariant:
      if the DT is not initialized, its count should be 0. */
   if(!bIsInitialized) {
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }
   }

   /* Invariant: if DT not initialized, root node should be NULL. */
   if(!bIsInitialized) {
      if (oNRoot != NULL) {
         fprintf(stderr, 
         "Not initialized, but root node is not NULL\n");
         return FALSE;
      }
   }

   /* Invariant: if oNRoot is NULL, DT count should be 0. */
   if (oNRoot == NULL) {
      if (ulCount != 0) {
         fprintf(stderr, "Root node is NULL, but count is not 0\n");
         return FALSE;
      }
   }

   /* Invariant: if oNRoot is not NULL, DT count should not be 0. */
   if (oNRoot != NULL) {
      if (ulCount == 0) {
         fprintf(stderr, "Root node is not NULL, but count is 0\n");
         return FALSE;
      }
   }

   /* Checks invariants recursively at each node from the root. */
   /* dtBad4 invariant: checks if number of nodes counted matchees 
   ulCount of DT */
   nodeCount = 0;
   nodeCount = CheckerDT_treeCheck(oNRoot,nodeCount);
   if (nodeCount != ulCount) {
      fprintf(stderr, "ulCount does not match actual number \
of nodes in the DT. ulCount = %lu. \
Number of nodes counted: %lu.\n",ulCount,nodeCount);
      return FALSE;
   }
   return TRUE;
}
