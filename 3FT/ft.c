/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: George Tziampazis, Will Huang                              */
/*--------------------------------------------------------------------*/

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynarray.h"
#include "path.h"
#include "noded.h"
#include "nodef.h"
#include "ft.h"

/*
  A File Tree is a representation of a hierarchy of directories and
  files: the File Tree is rooted at a directory, directories
  may be internal nodes or leaves, and files are always leaves. It is 
  represented as an AO with 3 state variables:
*/

/* Variables to keep track of FT characteristics: */
/* 1. Flag for being in initialized state (TRUE) or not (FALSE) */
static boolean bIsInitialized;
/* 2. Pointer to root directory node in the FT */
static NodeD_T oNRoot;
/* 3. Counter of number of directories (not including files) in FT */
static size_t ulDirCount;

/* --------------------------------------------------------------------

  The FT_traversePath, FT_findFile, and FT_findDir functions modularize 
  the common functionality of going as far as possible down an FT 
  towards a path and returning either the directory of however far was 
  reached (traversePath) or the node if the full path was reached 
  (findDir and findFile).
*/

/*
  Traverses the FT starting at the root to the farthest possible 
  DIRECTORY following absolute path oPPath. If able to traverse, 
  returns an int SUCCESS status and sets *poNFurthest to the furthest 
  directory node reached (which may be only a prefix of oPPath, the 
  entire oPPath, or even NULL if the root is NULL). Otherwise, sets 
  *poNFurthest to NULL and 
  returns with status:
  * CONFLICTING_PATH if the root's path is not a prefix of oPPath
  * MEMORY_ERROR if memory could not be allocated to complete request
 
  *Credit: Adapted from DT_traversePath() (Christopher Moretti)
*/
static int FT_traversePath(Path_T oPPath, NodeD_T *poNFurthest) {
    int iStatus;
    Path_T oPPrefix = NULL;
    NodeD_T oNCurr;
    NodeD_T oNChild;
    size_t ulDepth, ulChildID;
    size_t i;

    assert(oPPath != NULL);
    assert(poNFurthest != NULL);

    /* root is NULL -> won't find anything */
    if(oNRoot == NULL) {
        *poNFurthest = NULL;
        return SUCCESS;
    }

    /* checking depth of oPPath is valid */
    iStatus = Path_prefix(oPPath, 1, &oPPrefix);
    if(iStatus != SUCCESS) {
        *poNFurthest = NULL;
        return iStatus;
    }

    /* If the root in the given path is not the same as the actual root 
    of the FT */
    if(Path_comparePath(NodeD_getPath(oNRoot), oPPrefix)) {
        Path_free(oPPrefix);
        *poNFurthest = NULL;
        return CONFLICTING_PATH;
    }
    Path_free(oPPrefix);
    oPPrefix = NULL;

    oNCurr = oNRoot;
    ulDepth = Path_getDepth(oPPath);
    /* Increment over depths until at closest ancestor DIRECTORY of 
    last node in the path. If the last node is a directory, it will 
    stop there. */
    for (i = 2; i <= ulDepth; i++) {
        iStatus = Path_prefix(oPPath, i, &oPPrefix);
        if(iStatus != SUCCESS) {
            *poNFurthest = NULL;
            return iStatus;
        }
        /* If the current node has the next directory as a child */
        if (NodeD_hasDirChild(oNCurr, oPPrefix, &ulChildID)) {
            Path_free(oPPrefix);
            oPPrefix = NULL;
            iStatus = NodeD_getDirChild(oNCurr, ulChildID, &oNChild);
            if (iStatus != SUCCESS) {
                *poNFurthest = NULL;
                return iStatus;
            }
            /* Set up for next depth */
            oNCurr = oNChild;
        }
        else {
            break;
        }
    }
    Path_free(oPPrefix);
    *poNFurthest = oNCurr;
    return SUCCESS;
}

/* ================================================================== */
/*
  Traverses the FT to find a file with absolute path pcPath. Returns a 
  int SUCCESS status and sets *poNResult to be the node, if found. 
  Otherwise, sets *poNResult to NULL and returns with status:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if no node with pcPath exists in the hierarchy
  * MEMORY_ERROR if memory could not be allocated to complete request

  * The path pcPath is absolute, it must end in a file to be valid for 
  this function
 */
static int FT_findFile(const char *pcPath, NodeF_T *poNResult) {
    int iStatus;
    Path_T oPPath = NULL;
    Path_T oPParentPath = NULL;
    NodeD_T oNParent = NULL;
    NodeF_T oNFound = NULL;
    size_t ulChildID;

    assert(pcPath != NULL);
    assert(poNResult != NULL);

    /* Confirm that FT is initialized */
    if(!bIsInitialized) {
        *poNResult = NULL;
        return INITIALIZATION_ERROR;
    }

    /* validate pcPath and generate a Path_T for it */
    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) {
        *poNResult = NULL;
        return iStatus;
    }

    /* Find the directory parent of the file (if it exists) */
    iStatus = FT_traversePath(oPPath, &oNParent);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        *poNResult = NULL;
        return iStatus;
    }

    /* A file cannot have a NULL parent */
    if(oNParent == NULL) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    /* Checks that the correct path exists */
    iStatus = Path_prefix(oPPath,Path_getDepth(oPPath)-1,&oPParentPath);
    if(iStatus != SUCCESS) {
        Path_free(oPPath);
        *poNResult = NULL;
        return iStatus;
    }
    if(Path_comparePath(NodeD_getPath(oNParent), oPParentPath) != 0) {
        Path_free(oPParentPath);
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    /* If the parent has the file as a child, set it to oNFound */
    if (!NodeD_hasFileChild(oNParent, oPPath, &ulChildID)) {
        Path_free(oPPath);
        Path_free(oPParentPath);
        return NO_SUCH_PATH;
    }
    iStatus = NodeD_getFileChild(oNParent, ulChildID, &oNFound);
    if (iStatus != SUCCESS) {
        Path_free(oPParentPath);
        Path_free(oPPath);
        return iStatus;
    }   

    Path_free(oPPath);
    Path_free(oPParentPath);

    *poNResult = oNFound;
    return SUCCESS;
}

/* ================================================================== */
/*
  Traverses the FT to find a directory with absolute path pcPath. 
  Returns a int SUCCESS status and sets *poNResult to be the node, if 
  found. Otherwise, sets *poNResult to NULL and returns with status:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if no node with pcPath exists in the hierarchy
  * MEMORY_ERROR if memory could not be allocated to complete request

  * The path pcPath is absolute, it must end in a directory to be valid 
  for this function. This function is essentially a safer, more 
  specialized version of traverse path (more checks).
 */
static int FT_findDir(const char *pcPath, NodeD_T *poNResult) {
    Path_T oPPath = NULL;
    NodeD_T oNFound = NULL;
    NodeF_T oNFile = NULL;
    int iStatus;

    assert(pcPath != NULL);
    assert(poNResult != NULL);

    /* Confirm that FT is initialized */
    if(!bIsInitialized) {
        *poNResult = NULL;
        return INITIALIZATION_ERROR;
    }

    /* validate pcPath and generate a Path_T for it */
    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) {
        *poNResult = NULL;
        return iStatus;
    }

    /* Find the directory parent of the file (if it exists) */
    iStatus = FT_traversePath(oPPath, &oNFound);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        *poNResult = NULL;
        return iStatus;
    }

    /* Directory cannot be NULL, it won't be NULL even if it is the 
    root */
    if(oNFound == NULL) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    /* Checks that the node being searched for is NOT a file */
    iStatus = FT_findFile(pcPath,&oNFile);
    if (iStatus == SUCCESS) {
        Path_free(oPPath);
        return NOT_A_DIRECTORY;
    }

    /* Checks that found correct path */
    if(Path_comparePath(NodeD_getPath(oNFound), oPPath) != 0) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    Path_free(oPPath);
    *poNResult = oNFound;
    return SUCCESS;
}

/* ================================================================== */
int FT_insertDir(const char *pcPath) {
    int iStatus;
    Path_T oPPath = NULL;
    NodeD_T oNFirstNew = NULL;
    NodeD_T oNCurr = NULL;
    size_t ulDepth, ulIndex;
    size_t ulNewNodes = 0; 

    assert(pcPath != NULL);

    if(!bIsInitialized)
        return INITIALIZATION_ERROR;
    
    /* validate pcPath and generate a Path_T for it */
    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS)
        return iStatus;
    
    /* find the closest directory ancestor of oPPath already in the 
    tree, ancestor must be a directory by definition of file tree */
    iStatus= FT_traversePath(oPPath, &oNCurr);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        return iStatus;
    }

    /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
    if(oNCurr == NULL && oNRoot != NULL) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    ulDepth = Path_getDepth(oPPath);
    if(oNCurr == NULL) /* new root! */
        ulIndex = 1;
    else {
        ulIndex = Path_getDepth(NodeD_getPath(oNCurr)) + 1;
        /* oNCurr is the node we're trying to insert */
        if (ulIndex == ulDepth + 1 && !Path_comparePath(oPPath, 
        NodeD_getPath(oNCurr))) {
            Path_free(oPPath);
            return ALREADY_IN_TREE;
        }
    }

    /* starting at oNCurr, build rest of the path one level at a time */
    while (ulIndex <= ulDepth) {
        Path_T oPPrefix = NULL;
        NodeD_T oNNewNode = NULL;
        /* generate a Path_T for this level */
        iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
        if (iStatus != SUCCESS) {
            Path_free(oPPath);
            if (oNFirstNew != NULL)
                (void) NodeD_free(oNFirstNew);
            return iStatus;
        }
        /* If trying to insert child of a file */
        if (FT_containsFile(Path_getPathname(oPPrefix))) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if (oNFirstNew != NULL)
                (void)NodeD_free(oNFirstNew);
            return NOT_A_DIRECTORY;
        }
        /* insert the new node for this level */
        iStatus = NodeD_new(oPPrefix, oNCurr, &oNNewNode);
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if(oNFirstNew != NULL)
                (void) NodeD_free(oNFirstNew);
            return iStatus;
        }
        /* set up for next level */
        Path_free(oPPrefix);
        oNCurr = oNNewNode;
        ulNewNodes++;
        if(oNFirstNew == NULL)
            oNFirstNew = oNCurr;
        ulIndex++;
    }

    Path_free(oPPath);
    /* update FT state variables to reflect insertion */
    if(oNRoot == NULL)
        oNRoot = oNFirstNew;
    ulDirCount += ulNewNodes;

    return SUCCESS;
}   

/* ================================================================== */
boolean FT_containsDir(const char *pcPath) {
    int iStatus;
    NodeD_T oNFound = NULL;

    assert(pcPath != NULL);

    /* iStatus becomes SUCCESS if the directory is found, therefore is 
    contained in the FT */
    iStatus = FT_findDir(pcPath, &oNFound);
    return (boolean) (iStatus == SUCCESS);
}

/* ================================================================== */
int FT_rmDir(const char *pcPath) {
    int iStatus;
    NodeD_T oNdFound = NULL;

    assert(pcPath != NULL);

    /* pcPath is a path to a file */
    /*iStatus = FT_findFile(pcPath,&oNfFound);
    if (iStatus == SUCCESS) {
        return NOT_A_DIRECTORY;
    }*/

    /* Locate the directory and set it to oNdFound */
    iStatus = FT_findDir(pcPath, &oNdFound);
    if(iStatus != SUCCESS)
        return iStatus;

    /* Free the directory (including its children) */
    ulDirCount -= NodeD_free(oNdFound);
    if(ulDirCount == 0)
        oNRoot = NULL;

    return SUCCESS;
}

/* ================================================================== */
int FT_insertFile(const char *pcPath, void *pvContents, size_t 
ulLength) {
    int iStatus;
    Path_T oPPath = NULL; 
    NodeD_T oNFirstNew = NULL; /* first new node added */
    NodeD_T oNParent = NULL;
    NodeF_T oNNewFile = NULL; /* file to be added */
    size_t ulDepth, ulIndex, ulChildID; 
    size_t ulNewNodes = 0; /* number of new directories */

    assert(pcPath != NULL);

    if(!bIsInitialized)
        return INITIALIZATION_ERROR;
    
    /* validate pcPath and generate a Path_T for it */
    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS)
        return iStatus;
    
    if(Path_getDepth(oPPath) == 1) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }
    
    /* find the closest directory ancestor of oPPath already in the 
    tree, ancestor must be a directory by definition of file tree */
    iStatus= FT_traversePath(oPPath, &oNParent);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        return iStatus;
    }

    /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
    if(oNParent == NULL && oNRoot != NULL) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    ulDepth = Path_getDepth(oPPath);
    if (oNParent == NULL) /* new root! */
        ulIndex = 1;
    else {
        ulIndex = Path_getDepth(NodeD_getPath(oNParent)) + 1;
        /* the file is already a child of its parent directory */
        if (NodeD_hasFileChild(oNParent, oPPath, &ulChildID) || 
        NodeD_hasDirChild(oNParent, oPPath, &ulChildID) || 
        (Path_comparePath(NodeD_getPath(oNParent), oPPath) == 0)) {
            Path_free(oPPath);
            return ALREADY_IN_TREE;
        }
    }

    /* starting at oNParent, build rest of the directories one by one 
    but not the file itself yet, hence < not <= */
    while (ulIndex < ulDepth) {
        Path_T oPPrefix = NULL;
        NodeD_T oNNewNode = NULL;
        /* generate a Path_T for this level */
        iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
        if (iStatus != SUCCESS) {
            Path_free(oPPath);
            if (oNFirstNew != NULL)
                (void) NodeD_free(oNFirstNew);
            return iStatus;
        }
        /* If trying to insert child of a file */
        if (FT_containsFile(Path_getPathname(oPPrefix))) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if (oNFirstNew != NULL)
                (void)NodeD_free(oNFirstNew);
            return NOT_A_DIRECTORY;
        }
        /* insert the new node for this level */
        iStatus = NodeD_new(oPPrefix, oNParent, &oNNewNode);
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if(oNFirstNew != NULL)
                (void) NodeD_free(oNFirstNew);
            return iStatus;
        }
        /* set up for next level */
        Path_free(oPPrefix);
        oNParent = oNNewNode;
        ulNewNodes++;
        if(oNFirstNew == NULL)
            oNFirstNew = oNParent;
        ulIndex++;
    }
    
    /* generate a new node with only initialized fields */
    iStatus = NodeF_new(oPPath, &oNNewFile);
    if(iStatus != SUCCESS) {
        Path_free(oPPath);
        if(oNFirstNew != NULL)
            (void) NodeD_free(oNFirstNew);
        return iStatus;
    }
    
    /* Check if the file is already in the tree as a child of the 
    parent directory, if not add it as a child of the parent directory. 
    ulChildID is generated from NodeD_hasFileChild() */
    if (NodeD_hasFileChild(oNParent, oPPath, &ulChildID)) {
        Path_free(oPPath);
        free(oNNewFile);
        return ALREADY_IN_TREE;
    }
    iStatus = NodeD_addFileChild(oNParent, oNNewFile, ulChildID);
    if (iStatus != SUCCESS) {
        Path_free(oPPath);
        if(oNFirstNew != NULL)
            (void) NodeD_free(oNFirstNew);
        return iStatus; 
    }
    
    /* Set the fields of the new node */
    (void)NodeF_replaceContents(oNNewFile,pvContents);
    (void)(NodeF_replaceLength(oNNewFile,ulLength));

    Path_free(oPPath);
    /* update DT state variables to reflect insertion */
    if(oNRoot == NULL)
        oNRoot = oNFirstNew;
    /* Update the number of directories (not files, those are not 
    counted) */
    ulDirCount += ulNewNodes;

    return SUCCESS;
}

/* ================================================================== */
boolean FT_containsFile(const char *pcPath) {
    int iStatus;
    NodeF_T oNFound = NULL;

    assert(pcPath != NULL);

    /* iStatus == SUCCESS if file can be found */
    iStatus = FT_findFile(pcPath, &oNFound);
    return (boolean) (iStatus == SUCCESS);
}

/* ================================================================== */
int FT_rmFile(const char *pcPath) {
    int iStatus;
    size_t ulIndex;
    NodeF_T oNFound = NULL;
    NodeD_T oNdParent = NULL;

    assert(pcPath != NULL);

    /* pcPath is Path of a dir not a file */
    iStatus = FT_findDir(pcPath,&oNdParent);
    if(iStatus == SUCCESS) {
        return NOT_A_FILE;
    }

    /* Find file and assign to oNFound */
    iStatus = FT_findFile(pcPath, &oNFound);
    if(iStatus != SUCCESS)
        return iStatus;

    /* Find parent of the file */    
    iStatus = FT_traversePath(NodeF_getPath(oNFound), &oNdParent);
    if (iStatus != SUCCESS) {
        return iStatus;
    }
    /* Sets right ulIndex */
    (void)NodeD_hasFileChild(oNdParent, NodeF_getPath(oNFound),
    &ulIndex);

    /* Remove and free the file node */
    NodeF_free(oNFound);
    (void)DynArray_removeAt(NodeD_getFileChildren(oNdParent), ulIndex);

    return SUCCESS;
}

/* ================================================================== */
void *FT_getFileContents(const char *pcPath) {
    int iStatus;
    NodeF_T oNFound;

    assert(pcPath != NULL);

    /* Find the file and set to oNFound so contents can be accessed */
    iStatus = FT_findFile(pcPath, &oNFound);
    if(iStatus != SUCCESS)
        return NULL;

    return NodeF_getContents(oNFound);
}

/* ================================================================== */
void *FT_replaceFileContents(const char *pcPath, void *pvNewContents, 
size_t ulNewLength) {
    int iStatus;
    NodeF_T oNFound = NULL;

    assert(pcPath != NULL);

    /* Find file and set to oNFound so contents can be edited */
    iStatus = FT_findFile(pcPath, &oNFound);
    if(iStatus != SUCCESS)
        return NULL;
    
    (void)NodeF_replaceLength(oNFound,ulNewLength);
    return NodeF_replaceContents(oNFound,pvNewContents);
}

/* ================================================================== */
int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize) {
    NodeD_T oNdFound = NULL;
    NodeF_T oNfFound = NULL;
    int iStatusDir;
    int iStatusFile;

    assert(pcPath != NULL);

    /* Check if path can be found as a file AND as a directory */
    iStatusDir = FT_findDir(pcPath, &oNdFound);
    iStatusFile = FT_findFile(pcPath, &oNfFound);

    /* Case 1: path found as a directory */
    if (iStatusDir == SUCCESS) {
        *pbIsFile = (int) FALSE;
        return iStatusDir;
    }
    /* Case 2: path found as a file */
    else if (iStatusFile == SUCCESS) {
        *pbIsFile = (int) TRUE;
        *pulSize = NodeF_getLength(oNfFound);
        return iStatusFile;
    }
    /* failed both cases */
    else {
        return iStatusDir;
    }
}

/* ================================================================== */
int FT_init(void) {
    /* cannot init an already intialized FT */
    if(bIsInitialized)
        return INITIALIZATION_ERROR;

    /* Initialize fields */
    bIsInitialized = TRUE;
    oNRoot = NULL;
    ulDirCount = 0;

    return SUCCESS;
}

/* ================================================================== */
int FT_destroy(void) {
    /* cannot destroy if it doesn't exist */
    if(!bIsInitialized)
        return INITIALIZATION_ERROR;

    /* Free from root if it exists (will recursively free everything 
    else)*/
    if(oNRoot) {
        ulDirCount -= NodeD_free(oNRoot);
        /* uninitialize root */
        oNRoot = NULL;
    }

    /* uninitialize FT fields */
    assert(ulDirCount == 0);

    bIsInitialized = FALSE;

    return SUCCESS;
}

/* ================================================================== */
/*
  The following auxiliary functions are used for generating the
  string representation of the FT.
*/

/*
  Performs pre-order traversal of file tree rooted at n,
  inserting each payload to DynArray_T d beginning at index i.
  Returns the next unused index in d after the insertion(s).
*/

static size_t FT_preOrderTraversal(NodeD_T n, DynArray_T d, size_t i) {
    size_t c;

    assert(d != NULL);

    if(n != NULL) {
        (void) DynArray_set(d, i, n);
        i++;
        for(c = 0; c < NodeD_getNumDirChildren(n); c++) {
            int iStatus;
            NodeD_T oNdChild = NULL;
            iStatus = NodeD_getDirChild(n,c, &oNdChild);
            assert(iStatus == SUCCESS);
            i = FT_preOrderTraversal(oNdChild, d, i);
        }
    }
    return i;
}

/*
  Alternate version of strlen that uses pulAcc as an in-out parameter
  to accumulate a string length, rather than returning the length of
  oNdNode's path, and also always adds one addition byte to the sum.
  Also accumulates the lengths of oNdNode's children Paths.
*/
static void FT_strlenAccumulate(NodeD_T oNdNode, size_t *pulAcc) {
    char* pcNodeString;

    assert(pulAcc != NULL);

    /* Accumulate string lengths */
    if(oNdNode != NULL) {
        pcNodeString = NodeD_toString(oNdNode);
        *pulAcc += strlen(pcNodeString);
        free(pcNodeString);      
    }
}

/*
  Alternate version of strcat that inverts the typical argument
  order, appending oNdNode's string represenation onto pcAcc, and also 
  always adds one newline at the end of the concatenated string.
*/
static void FT_strcatAccumulate(NodeD_T oNdNode, char *pcAcc) {
    char* pcNodeString;

    assert(pcAcc != NULL);

    if(oNdNode != NULL) {
        pcNodeString = NodeD_toString(oNdNode);
        strcat(pcAcc, pcNodeString);
        free(pcNodeString);
    }
}

/* ================================================================== */
char *FT_toString(void) {
    DynArray_T nodes;
    size_t totalStrlen = 1;
    char *result = NULL;
    
    /* Make sure FT is initialized */
    if(!bIsInitialized)
        return NULL;

    /* Create array of all directory nodes to accumulate them */
    nodes = DynArray_new(ulDirCount);
    (void) FT_preOrderTraversal(oNRoot, nodes, 0);

    /* Accumulate length of all directory node strings */
    DynArray_map(nodes, (void (*)(void *, void*)) FT_strlenAccumulate,
                    (void*) &totalStrlen);

    /* Allocate mem and check for enough mem */
    result = malloc(totalStrlen + 1);
    if(result == NULL) {
        DynArray_free(nodes);
        return NULL;
    }
    *result = '\0';

    /* Accumulate string representations of all nodes */
    DynArray_map(nodes, (void (*)(void *, void*)) FT_strcatAccumulate,
    (void *) result);
    
    DynArray_free(nodes);

    return result;
}