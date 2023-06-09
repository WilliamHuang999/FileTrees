/*--------------------------------------------------------------------*/
/* noded.h                                                            */
/* Author: Will Huang and George Tziampazis                           */
/*--------------------------------------------------------------------*/

#ifndef NODED_INCLUDED
#define NODED_INCLUDED

#include <stddef.h>
#include "a4def.h"
#include "path.h"
#include "nodef.h"


/* A NodeD_T is a node in a Directory Tree */
typedef struct nodeD *NodeD_T;

/*
  Creates a new directory node in Directory Tree, with path oPPath and
  parent oNdParent. Returns an int SUCCESS status and sets *poNdResult
  to be the new node if successful. Otherwise, sets *poNdResult to NULL
  and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNdParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNdParent's path is not oPPath's direct parent
                 or oNdParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNdParent already has a child with this path
*/
int NodeD_new(Path_T oPPath, NodeD_T oNdParent, NodeD_T *poNdResult);

/*
  Destroys and frees all memory allocated for the subtree rooted at
  oNdNode, i.e., deletes this directory and all its descendents. 
  Returns the number of directories (exluding files) deleted.
*/
size_t NodeD_free(NodeD_T oNdNode);

/*
  Links new file child oNfChild into oNdParent's file children array at index ulIndex. Returns SUCCESS if the new directory child was added successfully, or  MEMORY_ERROR if allocation fails adding oNdChild to the directory children array.
*/
int NodeD_addFileChild(NodeD_T oNdParent, NodeF_T oNfChild, size_t ulIndex);


/* Returns the path object representing oNdNode's absolute path. */
Path_T NodeD_getPath(NodeD_T oNdNode);

/*
  Returns TRUE if oNdParent has a child directory with path oPPath. Returns FALSE if it does not.

  If oNdParent has such a child, stores in *pulChildID the child's
  identifier (as used in NodeD_getDirChild). If oNdParent does not have
  such a child, stores in *pulChildID the identifier that such a
  child _would_ have if inserted.
*/
boolean NodeD_hasDirChild(NodeD_T oNdParent, Path_T oPPath,
                         size_t *pulChildID);

/*
  Returns TRUE if oNdParent has a child file with path oPPath. Returns
  FALSE if it does not.

  If oNdParent has such a child, stores in *pulChildID the child's
  identifier (as used in NodeD_getFileChild). If oNdParent does not have
  such a child, stores in *pulChildID the identifier that such a
  child _would_ have if inserted.
*/
boolean NodeD_hasFileChild(NodeD_T oNdParent, Path_T oPPath,
                         size_t *pulChildID);

/* Returns the number of directory children that oNdParent has. */
size_t NodeD_getNumDirChildren(NodeD_T oNdParent);

/* Returns the number of file children that oNdParent has. */
size_t NodeD_getNumFileChildren(NodeD_T oNdParent);


/*
  Returns an int SUCCESS status and sets *poNdResult to be the directory child node of oNdParent with identifier ulChildID, if one exists. Otherwise, sets *poNdResult to NULL and returns status:
  * NO_SUCH_PATH if ulChildID is not a valid child for oNdParent
*/
int NodeD_getDirChild(NodeD_T oNdParent, size_t ulChildID,
                   NodeD_T *poNdResult);

/*
  Returns an int SUCCESS status and sets *poNfResult to be the file child node of oNdParent with identifier ulChildID, if one exists.
  Otherwise, sets *poNfResult to NULL and returns status:
  * NO_SUCH_PATH if ulChildID is not a valid child for oNdParent
*/
int NodeD_getFileChild(NodeD_T oNdParent, size_t ulChildID,
                   NodeF_T *poNfResult);

/*
  Returns a the parent node of oNdNode.
  Returns NULL if oNDNode is the root and thus has no parent.
*/
NodeD_T NodeD_getParent(NodeD_T oNdNode);

/*
  Compares two directory nodes oNdNode1 and oNdNode2 lexicographically 
  based on their paths. Returns <0, 0, or >0 if oNdNode1 is "less 
  than", "equal to", or "greater than" oNdNode2, respectively.
*/
int NodeD_compare(NodeD_T oNdNode1, NodeD_T oNdNode2);

/*
  Returns a string representation for oNdNode, or NULL if
  there is an allocation error. String representation includes the file children.

  Allocates memory for the returned string, which is then owned by
  the caller!
*/
char *NodeD_toString(NodeD_T oNdNode);

/* Returns the dynarray object representing the children of oNdNode that are files */
DynArray_T NodeD_getFileChildren(NodeD_T oNdNode);

/* Returns the dynarray object representing the children of oNdNode that are directories */
DynArray_T NodeD_getDirChildren(NodeD_T oNdNode);

#endif
