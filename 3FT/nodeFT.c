/*--------------------------------------------------------------------*/
/* nodeFT.c                                                           */
/* A node representing either a file or directory in a File Tree      */
/*--------------------------------------------------------------------*/

#include <stddef.h>
#include "a4def.h"
#include "path.h"
#include "nodeFT.h"

/* A Node_T is a pointer to a node in the File Tree. */
struct node {
    Path_T oPPath;
    Node_T oNParent;
    DynArray_T oChildren;
    NodeType eType;
    char *pcContents; /* Only for files, NULL for directories */
};

/* Enum to indicate node type (file or directory). */
enum { FT_DIR, FT_FILE } NodeType;

/*
  Creates a new node in the File Tree with path oPPath, parent oNParent, and type.
  Returns an int SUCCESS status and sets *poNResult to the new node if successful.
  Otherwise:
  - MEMORY_ERROR on allocation failure
  - CONFLICTING_PATH if oNParent’s path is not an ancestor of oPPath
  - NO_SUCH_PATH if oPPath depth is 0, or parent/child relationship is invalid
  - ALREADY_IN_TREE if a sibling with the same path already exists
*/
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "nodeFT.h"
#include "dynarray.h"

/* Internal structure of a node in the File Tree */
struct node {
    Path_T oPPath;           /* The absolute path of the node */
    Node_T oNParent;         /* Pointer to parent node (NULL for root) */
    DynArray_T oChildren;    /* Array of child nodes (only for directories) */
    NodeType eType;          /* FT_FILE or FT_DIR */
    char *pcContents;        /* String contents (only for files) */
};

/*
  Creates a new node in the File Tree with path oPPath, parent oNParent, and type.
  Returns an int SUCCESS status and sets *poNResult to the new node if successful.
  Otherwise:
  - MEMORY_ERROR on allocation failure
  - CONFLICTING_PATH if oNParent’s path is not an ancestor of oPPath
  - NO_SUCH_PATH if oPPath depth is 0, or parent/child relationship is invalid
  - ALREADY_IN_TREE if a sibling with the same path already exists
*/
int Node_new(Path_T oPPath, Node_T oNParent, NodeType eType, Node_T *poNResult) {
    /* Index where the child would be inserted (or found) in parent’s DynArray */
    size_t ulChildID;

    /* Ensure that the output parameter is valid */
    assert(poNResult != NULL);

    /* Check if the path has depth 0 (invalid in this tree structure) */
    if (Path_getDepth(oPPath) == 0)
        return NO_SUCH_PATH;

    if (oNParent == NULL) {
        /* If this is the root node, its path must be at depth 1 */
        if (Path_getDepth(oPPath) != 1)
            return NO_SUCH_PATH;
    } else {
        /* Get the parent’s path to perform relationship checks */
        Path_T oParentPath = Node_getPath(oNParent);

        /* Child’s path must be exactly one level deeper than parent’s */
        if (Path_getDepth(oPPath) != Path_getDepth(oParentPath) + 1)
            return NO_SUCH_PATH;

        /* Child’s path must start with the parent’s path as a prefix */
        if (!Path_prefix(oParentPath, oPPath))
            return CONFLICTING_PATH;

        /* Check if this path already exists as a child under this parent */
        if (Node_hasChild(oNParent, oPPath, &ulChildID))
            return ALREADY_IN_TREE;
    }

    /* Allocate memory for the new node structure */
    Node_T oNResult = malloc(sizeof(struct node));
    if (oNResult == NULL)
        return MEMORY_ERROR;

    /* Deep copy the provided path so it’s owned by the node */
    oNResult->oPPath = Path_copy(oPPath);
    if (oNResult->oPPath == NULL) {
        free(oNResult); /* Clean up partial allocation */
        return MEMORY_ERROR;
    }

    /* Set the node's parent */
    oNResult->oNParent = oNParent;

    /* Set the type: either FT_DIR or FT_FILE */
    oNResult->eType = eType;

    if (eType == FT_DIR) {
        /* If it's a directory, initialize an empty children array */
        oNResult->oChildren = DynArray_new(0);
        if (oNResult->oChildren == NULL) {
            Path_free(oNResult->oPPath);
            free(oNResult);
            return MEMORY_ERROR;
        }

        /* Directories do not store content */
        oNResult->pcContents = NULL;

    } else {
        /* If it's a file, it has no children, so set to NULL */
        oNResult->oChildren = NULL;

        /* Allocate a 1-byte string (empty file contents) */
        oNResult->pcContents = calloc(1, sizeof(char));  // ensures null-terminated
        if (oNResult->pcContents == NULL) {
            Path_free(oNResult->oPPath);
            free(oNResult);
            return MEMORY_ERROR;
        }
    }

    /* Store the pointer to the created node in the caller-provided location */
    *poNResult = oNResult;

    /* Success! Return the success status */
    return SUCCESS;
}


/*
  Frees all memory allocated for the subtree rooted at oNNode.
  Deletes this node and all its descendants.
  Returns the number of nodes freed.
*/
size_t Node_free(Node_T oNNode) {
    size_t ulTotalFreed = 0;  // Counter for number of nodes freed

    /* Sanity check: node must not be NULL */
    if (oNNode == NULL)
        return 0;

    /* If the node is a directory, recursively free all its children */
    if (Node_getType(oNNode) == FT_DIR) {
        size_t ulNumChildren = Node_getNumChildren(oNNode);

        /* Loop through each child in the directory */
        for (size_t i = 0; i < ulNumChildren; i++) {
            Node_T oChild;

            /* Retrieve the child node at index i */
            int status = Node_getChild(oNNode, i, &oChild);

            /* Sanity check: continue only if child exists */
            if (status == SUCCESS && oChild != NULL) {
                /* Recursively free this child and all of its descendants */
                ulTotalFreed += Node_free(oChild);
            }
        }

        /* Free the dynamic array of children itself */
        DynArray_free(oNNode->oChildren);
    }

    /* If the node is a file, free its contents */
    else if (Node_getType(oNNode) == FT_FILE) {
        free(oNNode->pcContents);
        oNNode->pcContents = NULL;  // Clear pointer (not required but good practice)
    }

    /* Free the Path_T associated with this node */
    Path_free(oNNode->oPPath);
    oNNode->oPPath = NULL;

    /* Free the node structure itself */
    free(oNNode);
    oNNode = NULL;

    /* Count this node as freed and return the total */
    return ulTotalFreed + 1;
}

/* Returns the absolute path associated with oNNode. */
Path_T Node_getPath(Node_T oNNode) {
    /* Assert that the node is not NULL to avoid dereferencing a bad pointer */
    assert(oNNode != NULL);

    /* Return the internal Path_T stored in the node structure */
    return oNNode->oPPath;
}
/* Returns the parent of oNNode, or NULL if it is the root. */
Node_T Node_getParent(Node_T oNNode) {
    /* Ensure the input node is valid (non-NULL) */
    assert(oNNode != NULL);

    /* Return the internal parent pointer stored in the node */
    return oNNode->oNParent;
}
/* Returns the number of children that oNParent has. */
size_t Node_getNumChildren(Node_T oNParent) {
    /* Check that the node is valid (not NULL) */
    assert(oNParent != NULL);

    /* Check that this is a directory — only directories have children */
    assert(Node_getType(oNParent) == FT_DIR);

    /* Return the number of child nodes stored in the DynArray */
    return DynArray_getLength(oNParent->oChildren);
}

/*
  Gets the child of oNParent at index ulChildID.
  Returns:
  - SUCCESS and sets *poNResult on success
  - NO_SUCH_PATH if index is invalid
*/
int Node_getChild(Node_T oNParent, size_t ulChildID, Node_T *poNResult) {
    /* Validate input pointers */
    assert(oNParent != NULL);
    assert(poNResult != NULL);

    /* Ensure the node is a directory (files don't have children) */
    assert(Node_getType(oNParent) == FT_DIR);

    /* Get the number of children for bounds checking */
    size_t numChildren = DynArray_getLength(oNParent->oChildren);

    /* Check if the index is valid */
    if (ulChildID >= numChildren) {
        *poNResult = NULL;      // Defensive: clear output
        return NO_SUCH_PATH;
    }

    /* Retrieve the child node at the given index */
    *poNResult = DynArray_get(oNParent->oChildren, ulChildID);

    /* Return success status */
    return SUCCESS;
}

/*
  Checks whether oNParent has a child matching path oPPath.
  Sets *pulChildID to the child's index if found, or where it would be inserted.
  Returns TRUE if found, FALSE otherwise.
*/
boolean Node_hasChild(Node_T oNParent, Path_T oPPath) {
    size_t numChildren;
    size_t i;

    /* Validate input pointers */
    assert(oNParent != NULL);
    assert(oPPath != NULL);
    assert(Node_getType(oNParent) == FT_DIR);  // Only directories have children

    /* Get the number of children in this directory */
    numChildren = DynArray_getLength(oNParent->oChildren);

    /* Loop through each child node and compare its path */
    for (i = 0; i < numChildren; i++) {
        Node_T child = DynArray_get(oNParent->oChildren, i);
        assert(child != NULL);  // Defensive check

        /* Compare the paths using Path_compare() */
        if (Path_compare(Node_getPath(child), oPPath) == 0) {
            return TRUE;  // Found a match
        }
    }

    /* No matching child was found */
    return FALSE;
}
/*
  Compares two nodes' paths lexicographically.
  Returns < 0, 0, or > 0 depending on order.
*/
int Node_compare(Node_T oNFirst, Node_T oNSecond) {
    /* Validate input pointers */
    assert(oNFirst != NULL);
    assert(oNSecond != NULL);

    /* Get the paths associated with both nodes */
    Path_T pFirst = Node_getPath(oNFirst);
    Path_T pSecond = Node_getPath(oNSecond);

    assert(pFirst != NULL);
    assert(pSecond != NULL);

    /* Use Path_compare to determine the ordering */
    return Path_compare(pFirst, pSecond);
}

/*
  Returns a malloc'd string representation of the node.
  Caller is responsible for freeing it.
  Returns NULL on memory allocation failure.
*/
char *Node_toString(Node_T oNNode) {
    assert(oNNode != NULL);

    /* Get the string representation of the path */
    char *pathStr = Path_toString(Node_getPath(oNNode));
    if (pathStr == NULL) return NULL;

    /* Get the type string explicitly (no ternary operator) */
    const char *typeStr;
    if (Node_getType(oNNode) == FT_FILE) {
        typeStr = "file";
    } else {
        typeStr = "dir";
    }

    /* Compute total length: path + " [type]" + null terminator */
    size_t len = strlen(pathStr) + strlen(" [file]") + 1;

    /* Allocate memory for the final string */
    char *result = malloc(len);
    if (result == NULL) {
        free(pathStr);
        return NULL;
    }

    /* Format the string */
    sprintf(result, "%s [%s]", pathStr, typeStr);

    /* Clean up path string */
    free(pathStr);

    return result;
}
/* ----------- FT-Specific Additions Below ----------- */

/*
  Returns the type of the node: FT_FILE or FT_DIR.
*/
NodeType Node_getType(Node_T oNNode) {
    /* Ensure the input node is valid */
    assert(oNNode != NULL);

    /* Return the internal type field (FT_FILE or FT_DIR) */
    return oNNode->eType;
}

/*
  Sets the contents of a file node.
  Overwrites existing contents if present.
  Returns:
  - 1 on success
  - 0 on failure or if node is not a file
*/
int Node_setContents(Node_T oNNode, const char *pcContents) {
    /* Ensure the input node is valid */
    assert(oNNode != NULL);

    /* Only file nodes are allowed to have contents */
    if (Node_getType(oNNode) != FT_FILE) {
        return 0;
    }

    /* Treat NULL content as an empty string */
    if (pcContents == NULL) {
        pcContents = "";
    }

    /* Allocate memory for the new contents (+1 for null terminator) */
    char *newContents = malloc(strlen(pcContents) + 1);
    if (newContents == NULL) {
        return 0;  // Allocation failed
    }

    /* Copy the new contents into the allocated memory */
    strcpy(newContents, pcContents);

    /* Free the old contents if any */
    free(oNNode->pcContents);

    /* Update the node with the new contents */
    oNNode->pcContents = newContents;

    return 1;  // Success
}

/*
  Returns the contents of a file node, or NULL if the node is not a file.
*/
const char *Node_getContents(Node_T oNNode) {
    /* Ensure the node is not NULL */
    assert(oNNode != NULL);

    /* Only file nodes have contents — return NULL for directories */
    if (Node_getType(oNNode) != FT_FILE) {
        return NULL;
    }

    /* Return the pointer to the contents string (may be empty) */
    return oNNode->pcContents;
}

int Node_removeChild(Node_T oParent, Node_T oChild) {
    assert(oParent != NULL && oChild != NULL);
    size_t numChildren = DynArray_getLength(oParent->oChildren);
    for (size_t i = 0; i < numChildren; i++) {
        Node_T child = DynArray_get(oParent->oChildren, i);
        if (child == oChild) {
            DynArray_removeAt(oParent->oChildren, i);
            return SUCCESS;
        }
    }
    return NO_SUCH_PATH;
}


