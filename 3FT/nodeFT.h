/*--------------------------------------------------------------------*/
/* nodeFT.h                                                           */
/* A node representing either a file or directory in a File Tree      */
/*--------------------------------------------------------------------*/

#ifndef NODEFT_INCLUDED
#define NODEFT_INCLUDED

#include <stddef.h>
#include "a4def.h"
#include "path.h"

/* A Node_T is a pointer to a node in the File Tree. */
typedef struct node *Node_T;

/* Enum to indicate node type (file or directory). */
typedef enum { FT_DIR, FT_FILE } NodeType;

/*
  Creates a new node in the File Tree with path oPPath, parent oNParent, and type.
  Returns an int SUCCESS status and sets *poNResult to the new node if successful.
  Otherwise:
  - MEMORY_ERROR on allocation failure
  - CONFLICTING_PATH if oNParent’s path is not an ancestor of oPPath
  - NO_SUCH_PATH if oPPath depth is 0, or parent/child relationship is invalid
  - ALREADY_IN_TREE if a sibling with the same path already exists
*/
int Node_new(Path_T oPPath, Node_T oNParent, NodeType type, Node_T *poNResult);

/*
  Frees all memory allocated for the subtree rooted at oNNode.
  Deletes this node and all its descendants.
  Returns the number of nodes freed.
*/
size_t Node_free(Node_T oNNode);

/* Returns the absolute path associated with oNNode. */
Path_T Node_getPath(Node_T oNNode);

/* Returns the parent of oNNode, or NULL if it is the root. */
Node_T Node_getParent(Node_T oNNode);

/* Returns the number of children that oNParent has. */
size_t Node_getNumChildren(Node_T oNParent);

/*
  Gets the child of oNParent at index ulChildID.
  Returns:
  - SUCCESS and sets *poNResult on success
  - NO_SUCH_PATH if index is invalid
*/
int Node_getChild(Node_T oNParent, size_t ulChildID, Node_T *poNResult);

/*
  Checks whether oNParent has a child matching path oPPath.
  Sets *pulChildID to the child's index if found, or where it would be inserted.
  Returns TRUE if found, FALSE otherwise.
*/
boolean Node_hasChild(Node_T oNParent, Path_T oPPath, size_t *pulChildID);

/*
  Compares two nodes' paths lexicographically.
  Returns < 0, 0, or > 0 depending on order.
*/
int Node_compare(Node_T oNFirst, Node_T oNSecond);

/*
  Returns a malloc'd string representation of the node.
  Caller is responsible for freeing it.
  Returns NULL on memory allocation failure.
*/
char *Node_toString(Node_T oNNode);

/* ----------- FT-Specific Additions Below ----------- */

/*
  Returns the type of the node: FT_FILE or FT_DIR.
*/
NodeType Node_getType(Node_T oNNode);

/*
  Sets the contents of a file node.
  Overwrites existing contents if present.
  Returns:
  - 1 on success
  - 0 on failure or if node is not a file
*/
int Node_setContents(Node_T oNNode, const char *pcContents);

/*
  Returns the contents of a file node, or NULL if the node is not a file.
*/
const char *Node_getContents(Node_T oNNode);

#endif
