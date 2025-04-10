/*--------------------------------------------------------------------*/
/* ft.h                                                               */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

/*
  A File Tree is a representation of a hierarchy of directories and
  files: the File Tree is rooted at a directory, directories
  may be internal nodes or leaves, and files are always leaves.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "a4def.h"
#include "path.h"
#include "dynarray.h"
#include "nodeFT.h"
#include "ft.h"
#include <string.h>

/* Forward declaration since path.h doesn't expose it */
int Path_comparePath(Path_T oPPath1, Path_T oPPath2);

static boolean bIsInitialized = FALSE;   /* Indicates if FT is initialized */
static Node_T oRoot = NULL;              /* The root node of the File Tree */

/*
   Inserts a new directory into the FT with absolute path pcPath.
   Returns SUCCESS if the new directory is inserted successfully.
   Otherwise, returns:
   * INITIALIZATION_ERROR if the FT is not in an initialized state
   * BAD_PATH if pcPath does not represent a well-formatted path
   * CONFLICTING_PATH if the root exists but is not a prefix of pcPath
   * NOT_A_DIRECTORY if a proper prefix of pcPath exists as a file
   * ALREADY_IN_TREE if pcPath is already in the FT (as dir or file)
   * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_insertDir(const char *pcPath) {
    Path_T oNewPath;
    Node_T oCurr = NULL;
    Node_T oNext = NULL;
    size_t depth, i;
    int result;

    /* ------------------ STEP 1: Error Checking ------------------ */

    /* If FT is not initialized, return error */
    if (!bIsInitialized)
        return INITIALIZATION_ERROR;

    /* Attempt to create a Path_T object from the string */
    oNewPath = Path_create(pcPath);
    if (oNewPath == NULL)
        return BAD_PATH;  // Path was malformed or NULL

    /* Get the depth of the new path (how many components like a/b/c = 3) */
    depth = Path_getDepth(oNewPath);

    /* Special case: empty path */
    if (depth == 0) {
        Path_free(oNewPath);
        return BAD_PATH;
    }

    /* ------------------ STEP 2: Handle empty tree (root creation) ------------------ */

    if (oRoot == NULL) {
        /* If this is the very first insertion, path must be depth 1 */
        if (depth != 1) {
            Path_free(oNewPath);
            return CONFLICTING_PATH; // Root doesn't exist yet, but path isn't direct root
        }

        /* Create the root node (parent is NULL, type is directory) */
        result = Node_new(oNewPath, NULL, FT_DIR, &oRoot);
        Path_free(oNewPath); // Path copied into node, we can free this one

        if (result != SUCCESS) {
            return result; // Could be MEMORY_ERROR or similar
        }

        return SUCCESS; // Root successfully inserted
    }

    /* ------------------ STEP 3: Traverse the path one level at a time ------------------ */

    oCurr = oRoot;

    /* Loop through each prefix of the path EXCEPT the last one */
    for (i = 1; i < depth; i++) {
Path_T oPrefix = NULL; // will hold the result
int prefixStatus = Path_prefix(oNewPath, i, &oPrefix);
if (prefixStatus != SUCCESS) {
    Path_free(oNewPath);
    return MEMORY_ERROR;
}

        /* Ensure we successfully created the prefix */
        if (oPrefix == NULL) {
            Path_free(oNewPath);
            return MEMORY_ERROR;
        }

        /* Check if current node has a child with this path */
        size_t numChildren = Node_getNumChildren(oCurr);
        boolean found = FALSE;

        for (size_t j = 0; j < numChildren; j++) {
            Node_T child;
            if (Node_getChild(oCurr, j, &child) == SUCCESS &&
                Path_comparePath(Node_getPath(child), oPrefix) == 0) {
                oNext = child;
                found = TRUE;
                break;
            }
        }

        Path_free(oPrefix);

        if (!found) {
            /* No such prefix found → missing intermediate directory */
            Path_free(oNewPath);
            return NO_SUCH_PATH;
        }

        /* Make sure this prefix is a directory */
        if (Node_getType(oNext) != FT_DIR) {
            Path_free(oNewPath);
            return NOT_A_DIRECTORY;
        }

        /* Move to next node down */
        oCurr = oNext;
    }

    /* ------------------ STEP 4: Check for conflicts at final level ------------------ */

    /* Check if the current node already has a child at the final component */
    size_t numChildren = Node_getNumChildren(oCurr);
    for (size_t j = 0; j < numChildren; j++) {
        Node_T child;
        if (Node_getChild(oCurr, j, &child) == SUCCESS &&
            Path_comparePath(Node_getPath(child), oNewPath) == 0) {
            Path_free(oNewPath);
            return ALREADY_IN_TREE;  // Already exists as dir or file
        }
    }

    /* ------------------ STEP 5: Create the new directory node ------------------ */

    Node_T oNewNode;
    result = Node_new(oNewPath, oCurr, FT_DIR, &oNewNode);

    Path_free(oNewPath);  // Done with the path object

    if (result != SUCCESS) {
        return result;  // Propagate error (e.g. MEMORY_ERROR)
    }

    /* Add new node to current node’s children */
    if (DynArray_add(oCurr->oChildren, oNewNode) == FALSE) {
        Node_free(oNewNode);
        return MEMORY_ERROR;
    }

    return SUCCESS;
}

/*
  Returns TRUE if the FT contains a directory with absolute path
  pcPath and FALSE if not or if there is an error while checking.
*/
boolean FT_containsDir(const char *pcPath) {
    Path_T oTargetPath;
    Node_T oCurr;
    Node_T oNext;
    size_t depth, i;

    /* ------------------ STEP 1: Basic checks ------------------ */

    /* FT must be initialized */
    if (!bIsInitialized)
        return FALSE;

    /* Create a Path_T object from the given path string */
    oTargetPath = Path_create(pcPath);
    if (oTargetPath == NULL)
        return FALSE;

    /* A valid directory must have depth ≥ 1 */
    depth = Path_getDepth(oTargetPath);
    if (depth == 0) {
        Path_free(oTargetPath);
        return FALSE;
    }

    /* If the tree is empty, nothing can be contained */
    if (oRoot == NULL) {
        Path_free(oTargetPath);
        return FALSE;
    }

    /* ------------------ STEP 2: Traverse the path ------------------ */

    oCurr = oRoot;

    /* Traverse from depth 1 up to full depth */
    for (i = 1; i <= depth; i++) {
        Path_T oPrefix = Path_prefix(oTargetPath, i);
        if (oPrefix == NULL) {
            Path_free(oTargetPath);
            return FALSE;
        }

        /* Assume not found unless we find a matching child */
        oNext = NULL;

        size_t numChildren = Node_getNumChildren(oCurr);
        for (size_t j = 0; j < numChildren; j++) {
            Node_T child;
            if (Node_getChild(oCurr, j, &child) == SUCCESS &&
                Path_comparePath(Node_getPath(child), oPrefix) == 0) {
                oNext = child;
                break;
            }
        }

        Path_free(oPrefix);

        /* If we didn't find a match, the path doesn't exist */
        if (oNext == NULL) {
            Path_free(oTargetPath);
            return FALSE;
        }

        /* Move one level deeper */
        oCurr = oNext;
    }

    /* ------------------ STEP 3: Check if it's a directory ------------------ */

    Path_free(oTargetPath);

    /* Final node exists — is it a directory? */
    if (Node_getType(oCurr) == FT_DIR) {
        return TRUE;
    } else {
        return FALSE;  // It's a file, not a directory
    }
}

/*
  Removes the FT hierarchy (subtree) at the directory with absolute
  path pcPath. Returns SUCCESS if found and removed.
  Otherwise, returns:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root exists but is not a prefix of pcPath
  * NO_SUCH_PATH if absolute path pcPath does not exist in the FT
  * NOT_A_DIRECTORY if pcPath is in the FT as a file not a directory
  * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_rmDir(const char *pcPath) {
    Path_T oTargetPath;
    Node_T oCurr = oRoot;
    Node_T oParent = NULL;
    Node_T oToRemove = NULL;
    size_t depth, i;

    /* ------------------ STEP 1: Basic validation ------------------ */

    if (!bIsInitialized)
        return INITIALIZATION_ERROR;

    oTargetPath = Path_create(pcPath);
    if (oTargetPath == NULL)
        return BAD_PATH;

    depth = Path_getDepth(oTargetPath);
    if (depth == 0) {
        Path_free(oTargetPath);
        return BAD_PATH;
    }

    if (oRoot == NULL) {
        Path_free(oTargetPath);
        return NO_SUCH_PATH;
    }

    /* ------------------ STEP 2: Special case – root deletion ------------------ */

    /* If we're deleting the root, it must match root's path exactly */
    if (Path_comparePath(Node_getPath(oRoot), oTargetPath) == 0) {
        if (Node_getType(oRoot) != FT_DIR) {
            Path_free(oTargetPath);
            return NOT_A_DIRECTORY;
        }

        /* Free the entire tree */
        Node_free(oRoot);
        oRoot = NULL;

        Path_free(oTargetPath);
        return SUCCESS;
    }

    /* ------------------ STEP 3: Traverse to the target node ------------------ */

    for (i = 1; i <= depth; i++) {
        Path_T oPrefix = Path_prefix(oTargetPath, i);
        if (oPrefix == NULL) {
            Path_free(oTargetPath);
            return MEMORY_ERROR;
        }

        Node_T oNext = NULL;
        size_t numChildren = Node_getNumChildren(oCurr);

        for (size_t j = 0; j < numChildren; j++) {
            Node_T child;
            if (Node_getChild(oCurr, j, &child) == SUCCESS &&
                Path_comparePath(Node_getPath(child), oPrefix) == 0) {
                oNext = child;
                break;
            }
        }

        Path_free(oPrefix);

        if (oNext == NULL) {
            Path_free(oTargetPath);
            return NO_SUCH_PATH;
        }

        oParent = oCurr;  // Save parent before moving deeper
        oCurr = oNext;
    }

    /* oCurr now points to the node we want to delete */
    oToRemove = oCurr;

    /* ------------------ STEP 4: Type check ------------------ */

    if (Node_getType(oToRemove) != FT_DIR) {
        Path_free(oTargetPath);
        return NOT_A_DIRECTORY;
    }

    /* ------------------ STEP 5: Remove node from parent's children ------------------ */

    size_t numChildren = Node_getNumChildren(oParent);
    for (size_t j = 0; j < numChildren; j++) {
        Node_T child;
        if (Node_getChild(oParent, j, &child) == SUCCESS &&
            child == oToRemove) {
            /* Remove child from array */
            DynArray_removeAt(oParent->oChildren, j);
            break;
        }
    }

    /* ------------------ STEP 6: Free the subtree ------------------ */

    Node_free(oToRemove);

    Path_free(oTargetPath);
    return SUCCESS;
}

/*
   Inserts a new file into the FT with absolute path pcPath, with
   file contents pvContents of size ulLength bytes.
   Returns SUCCESS if the new file is inserted successfully.
   Otherwise, returns:
   * INITIALIZATION_ERROR if the FT is not in an initialized state
   * BAD_PATH if pcPath does not represent a well-formatted path
   * CONFLICTING_PATH if the root exists but is not a prefix of pcPath,
                      or if the new file would be the FT root
   * NOT_A_DIRECTORY if a proper prefix of pcPath exists as a file
   * ALREADY_IN_TREE if pcPath is already in the FT (as dir or file)
   * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_insertFile(const char *pcPath, void *pvContents, size_t ulLength) {
    Path_T oNewPath;
    Node_T oCurr = NULL;
    Node_T oNext = NULL;
    size_t depth, i;
    int result;

    /* ------------------ STEP 1: Initialization & validation ------------------ */

    if (!bIsInitialized)
        return INITIALIZATION_ERROR;

    /* Parse the path string into a Path_T object */
    oNewPath = Path_create(pcPath);
    if (oNewPath == NULL)
        return BAD_PATH;

    depth = Path_getDepth(oNewPath);
    if (depth == 0) {
        Path_free(oNewPath);
        return BAD_PATH;
    }

    /* Cannot insert a file as the root */
    if (oRoot == NULL && depth == 1) {
        Path_free(oNewPath);
        return CONFLICTING_PATH;
    }

    /* ------------------ STEP 2: Handle first insertion (root must be a directory) ------------------ */

    if (oRoot == NULL) {
        Path_T rootPrefix = Path_prefix(oNewPath, 1);
        if (rootPrefix == NULL) {
            Path_free(oNewPath);
            return MEMORY_ERROR;
        }

        /* Create root as a directory */
        result = Node_new(rootPrefix, NULL, FT_DIR, &oRoot);
        Path_free(rootPrefix);
        if (result != SUCCESS) {
            Path_free(oNewPath);
            return result;
        }
    }

    oCurr = oRoot;

    /* ------------------ STEP 3: Traverse to parent of file ------------------ */

    for (i = 1; i < depth; i++) {
Path_T oPrefix = NULL;  // Declare the output variable
int status = Path_prefix(oNewPath, i, &oPrefix);  // Use &oPrefix to receive the result

if (status != SUCCESS) {
    Path_free(oNewPath);  // Clean up the input path
    return MEMORY_ERROR;  // Or whatever error you're using
}
        if (oPrefix == NULL) {
            Path_free(oNewPath);
            return MEMORY_ERROR;
        }

        boolean found = FALSE;
        size_t numChildren = Node_getNumChildren(oCurr);

        for (size_t j = 0; j < numChildren; j++) {
            Node_T child;
            if (Node_getChild(oCurr, j, &child) == SUCCESS &&
                Path_comparePath(Node_getPath(child), oPrefix) == 0) {
                oNext = child;
                found = TRUE;
                break;
            }
        }

        Path_free(oPrefix);

        if (!found) {
            Path_free(oNewPath);
            return CONFLICTING_PATH; // A prefix does not exist
        }

        /* Check that intermediate path is a directory */
        if (Node_getType(oNext) != FT_DIR) {
            Path_free(oNewPath);
            return NOT_A_DIRECTORY;
        }

        oCurr = oNext; // Move down one level
    }

    /* ------------------ STEP 4: Check if file already exists ------------------ */

    size_t numChildren = Node_getNumChildren(oCurr);
    for (size_t j = 0; j < numChildren; j++) {
        Node_T child;
        if (Node_getChild(oCurr, j, &child) == SUCCESS &&
            Path_comparePath(Node_getPath(child), oNewPath) == 0) {
            Path_free(oNewPath);
            return ALREADY_IN_TREE;
        }
    }

    /* ------------------ STEP 5: Create new file node ------------------ */

    Node_T oNewNode;
    result = Node_new(oNewPath, oCurr, FT_FILE, &oNewNode);
    Path_free(oNewPath);  // Done with path

    if (result != SUCCESS) {
        return result;
    }

    /* ------------------ STEP 6: Set file contents ------------------ */

    char *copy = malloc(ulLength);
    if (ulLength > 0 && copy == NULL) {
        Node_free(oNewNode);
        return MEMORY_ERROR;
    }

    /* Only copy if there’s something to copy */
    if (ulLength > 0 && pvContents != NULL) {
        memcpy(copy, pvContents, ulLength);
        if (!Node_setContents(oNewNode, copy)) {
            free(copy);
            Node_free(oNewNode);
            return MEMORY_ERROR;
        }
    } else {
        Node_setContents(oNewNode, NULL);  // Treat as empty
    }

    free(copy);  // We copied the contents into the node, safe to free

    /* ------------------ STEP 7: Add new file node to parent ------------------ */

    if (!DynArray_add(oCurr->oChildren, oNewNode)) {
        Node_free(oNewNode);
        return MEMORY_ERROR;
    }

    return SUCCESS;
}

/*
  Returns TRUE if the FT contains a file with absolute path
  pcPath and FALSE if not or if there is an error while checking.
*/
boolean FT_containsFile(const char *pcPath) {
    Path_T oTargetPath;
    Node_T oCurr = oRoot;
    Node_T oNext = NULL;
    size_t depth, i;

    /* ------------------ STEP 1: Validate state and input ------------------ */

    if (!bIsInitialized)
        return FALSE;

    /* Convert pcPath to a structured Path_T object */
    oTargetPath = Path_create(pcPath);
    if (oTargetPath == NULL)
        return FALSE;

    /* Get number of path components (e.g., /a/b/c → 3) */
    depth = Path_getDepth(oTargetPath);
    if (depth == 0) {
        Path_free(oTargetPath);
        return FALSE;
    }

    /* Empty tree — definitely not found */
    if (oRoot == NULL) {
        Path_free(oTargetPath);
        return FALSE;
    }

    /* ------------------ STEP 2: Traverse path one level at a time ------------------ */

    for (i = 1; i <= depth; i++) {
        /* Get the prefix of the path at this level */
        Path_T oPrefix = Path_prefix(oTargetPath, i);
        if (oPrefix == NULL) {
            Path_free(oTargetPath);
            return FALSE;
        }

        /* Look for a child of oCurr with the matching path */
        boolean found = FALSE;
        size_t numChildren = Node_getNumChildren(oCurr);

        for (size_t j = 0; j < numChildren; j++) {
            Node_T child;
            if (Node_getChild(oCurr, j, &child) == SUCCESS &&
                Path_comparePath(Node_getPath(child), oPrefix) == 0) {
                oNext = child;
                found = TRUE;
                break;
            }
        }

        Path_free(oPrefix);

        if (!found) {
            Path_free(oTargetPath);
            return FALSE;
        }

        oCurr = oNext;  // move down to the matched node
    }

    /* ------------------ STEP 3: Confirm that the final node is a file ------------------ */

    Path_free(oTargetPath);

    if (Node_getType(oCurr) == FT_FILE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
  Removes the FT file with absolute path pcPath.
  Returns SUCCESS if found and removed.
  Otherwise, returns:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root exists but is not a prefix of pcPath
  * NO_SUCH_PATH if absolute path pcPath does not exist in the FT
  * NOT_A_FILE if pcPath is in the FT as a directory not a file
  * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_rmFile(const char *pcPath) {
    Path_T oTargetPath;
    Node_T oCurr = oRoot;
    Node_T oParent = NULL;
    Node_T oNext = NULL;
    size_t depth, i;

    /* ------------------ STEP 1: Validate state and input ------------------ */

    if (!bIsInitialized)
        return INITIALIZATION_ERROR;

    oTargetPath = Path_create(pcPath);
    if (oTargetPath == NULL)
        return BAD_PATH;

    depth = Path_getDepth(oTargetPath);
    if (depth == 0) {
        Path_free(oTargetPath);
        return BAD_PATH;
    }

    if (oRoot == NULL) {
        Path_free(oTargetPath);
        return NO_SUCH_PATH;
    }

    /* ------------------ STEP 2: Cannot delete root as a file ------------------ */

    if (Path_comparePath(Node_getPath(oRoot), oTargetPath) == 0) {
        /* Root exists — check if it's a file (invalid) or directory */
        if (Node_getType(oRoot) == FT_FILE) {
            Path_free(oTargetPath);
            return NOT_A_FILE;
        } else {
            Path_free(oTargetPath);
            return CONFLICTING_PATH;  // It's the root dir, not allowed to remove via FT_rmFile
        }
    }

    /* ------------------ STEP 3: Traverse path to locate file ------------------ */

    for (i = 1; i <= depth; i++) {
        Path_T oPrefix = Path_prefix(oTargetPath, i);
        if (oPrefix == NULL) {
            Path_free(oTargetPath);
            return MEMORY_ERROR;
        }

        boolean found = FALSE;
        size_t numChildren = Node_getNumChildren(oCurr);

        for (size_t j = 0; j < numChildren; j++) {
            Node_T child;
            if (Node_getChild(oCurr, j, &child) == SUCCESS &&
                Path_comparePath(Node_getPath(child), oPrefix) == 0) {
                oNext = child;
                found = TRUE;
                break;
            }
        }

        Path_free(oPrefix);

        if (!found) {
            Path_free(oTargetPath);
            return NO_SUCH_PATH;
        }

        /* Save parent before stepping forward */
        oParent = oCurr;
        oCurr = oNext;
    }

    /* oCurr now points to the node we want to delete */

    /* ------------------ STEP 4: Type check ------------------ */

    if (Node_getType(oCurr) != FT_FILE) {
        Path_free(oTargetPath);
        return NOT_A_FILE;
    }

    /* ------------------ STEP 5: Remove from parent's children ------------------ */

    size_t numChildren = Node_getNumChildren(oParent);
    for (size_t j = 0; j < numChildren; j++) {
        Node_T child;
        if (Node_getChild(oParent, j, &child) == SUCCESS &&
            child == oCurr) {
            /* Remove it from the parent's children array */
            DynArray_removeAt(oParent->oChildren, j);
            break;
        }
    }

    /* ------------------ STEP 6: Free the file node ------------------ */

    Node_free(oCurr);
    Path_free(oTargetPath);

    return SUCCESS;
}

/*
  Returns the contents of the file with absolute path pcPath.
  Returns NULL if unable to complete the request for any reason.

  Note: checking for a non-NULL return is not an appropriate
  contains check, because the contents of a file may be NULL.
*/
void *FT_getFileContents(const char *pcPath) {
    Path_T oTargetPath;
    Node_T oCurr = oRoot;
    Node_T oNext = NULL;
    size_t depth, i;

    /* ------------------ STEP 1: Check initialization and path validity ------------------ */

    if (!bIsInitialized)
        return NULL;

    oTargetPath = Path_create(pcPath);
    if (oTargetPath == NULL)
        return NULL;

    depth = Path_getDepth(oTargetPath);
    if (depth == 0) {
        Path_free(oTargetPath);
        return NULL;
    }

    if (oRoot == NULL) {
        Path_free(oTargetPath);
        return NULL;
    }

    /* ------------------ STEP 2: Traverse the path to find the target node ------------------ */

    for (i = 1; i <= depth; i++) {
        Path_T oPrefix = Path_prefix(oTargetPath, i);
        if (oPrefix == NULL) {
            Path_free(oTargetPath);
            return NULL;
        }

        boolean found = FALSE;
        size_t numChildren = Node_getNumChildren(oCurr);

        for (size_t j = 0; j < numChildren; j++) {
            Node_T child;
            if (Node_getChild(oCurr, j, &child) == SUCCESS &&
                Path_comparePath(Node_getPath(child), oPrefix) == 0) {
                oNext = child;
                found = TRUE;
                break;
            }
        }

        Path_free(oPrefix);

        if (!found) {
            Path_free(oTargetPath);
            return NULL;  // One of the prefixes not found
        }

        oCurr = oNext;  // Move deeper
    }

    Path_free(oTargetPath);

    /* ------------------ STEP 3: Confirm it's a file and return contents ------------------ */

    if (Node_getType(oCurr) != FT_FILE) {
        return NULL;
    }

    /* Return pointer to the contents — may be NULL (empty file) */
    return (void *)Node_getContents(oCurr);
}

/*
  Replaces current contents of the file with absolute path pcPath with
  the parameter pvNewContents of size ulNewLength bytes.
  Returns the old contents if successful. (Note: contents may be NULL.)
  Returns NULL if unable to complete the request for any reason.
*/
void *FT_replaceFileContents(const char *pcPath, void *pvNewContents,
                             size_t ulNewLength) {
    Path_T oTargetPath;
    Node_T oCurr = oRoot;
    Node_T oNext = NULL;
    size_t depth, i;

    /* ------------------ STEP 1: Validate FT state and input path ------------------ */

    if (!bIsInitialized)
        return NULL;

    oTargetPath = Path_create(pcPath);
    if (oTargetPath == NULL)
        return NULL;

    depth = Path_getDepth(oTargetPath);
    if (depth == 0) {
        Path_free(oTargetPath);
        return NULL;
    }

    if (oRoot == NULL) {
        Path_free(oTargetPath);
        return NULL;
    }

    /* ------------------ STEP 2: Traverse tree to find the target file ------------------ */

    for (i = 1; i <= depth; i++) {
        Path_T oPrefix = Path_prefix(oTargetPath, i);
        if (oPrefix == NULL) {
            Path_free(oTargetPath);
            return NULL;
        }

        boolean found = FALSE;
        size_t numChildren = Node_getNumChildren(oCurr);

        for (size_t j = 0; j < numChildren; j++) {
            Node_T child;
            if (Node_getChild(oCurr, j, &child) == SUCCESS &&
                Path_comparePath(Node_getPath(child), oPrefix) == 0) {
                oNext = child;
                found = TRUE;
                break;
            }
        }

        Path_free(oPrefix);

        if (!found) {
            Path_free(oTargetPath);
            return NULL;
        }

        oCurr = oNext;
    }

    Path_free(oTargetPath);

    /* ------------------ STEP 3: Verify that the target is a file ------------------ */

    if (Node_getType(oCurr) != FT_FILE) {
        return NULL;
    }

    /* ------------------ STEP 4: Prepare new content ------------------ */

    char *newContents = NULL;
    if (ulNewLength > 0 && pvNewContents != NULL) {
        newContents = malloc(ulNewLength);
        if (newContents == NULL) {
            return NULL;  // Allocation failed
        }
        memcpy(newContents, pvNewContents, ulNewLength);
    }

    /* ------------------ STEP 5: Replace contents and return old contents ------------------ */

    void *oldContents = (void *)Node_getContents(oCurr);  // Save old pointer

    if (!Node_setContents(oCurr, newContents)) {
        /* Failed to set — free new content buffer and fail safely */
        free(newContents);
        return NULL;
    }

    return oldContents;  // Caller is responsible for freeing this if needed
}

/*
  Returns SUCCESS if pcPath exists in the hierarchy,
  Otherwise, returns:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if absolute path pcPath does not exist in the FT
  * MEMORY_ERROR if memory could not be allocated to complete request

  When returning SUCCESS,
  if path is a directory: sets *pbIsFile to FALSE, *pulSize unchanged
  if path is a file: sets *pbIsFile to TRUE, and
                     sets *pulSize to the length of file's contents

  When returning another status, *pbIsFile and *pulSize are unchanged.
*/
int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize) {
    Path_T oTargetPath;
    Node_T oCurr = oRoot;
    Node_T oNext = NULL;
    size_t depth, i;

    /* ------------------ STEP 1: Validate global state and input path ------------------ */

    if (!bIsInitialized)
        return INITIALIZATION_ERROR;

    oTargetPath = Path_create(pcPath);
    if (oTargetPath == NULL)
        return BAD_PATH;

    depth = Path_getDepth(oTargetPath);
    if (depth == 0) {
        Path_free(oTargetPath);
        return BAD_PATH;
    }

    if (oRoot == NULL) {
        Path_free(oTargetPath);
        return NO_SUCH_PATH;
    }

    /* ------------------ STEP 2: Handle potential conflict at root ------------------ */

    Path_T rootPath = Node_getPath(oRoot);
    if (!Path_prefix(rootPath, 1) || !Path_isPrefix(rootPath, oTargetPath)) {
        Path_free(oTargetPath);
        return CONFLICTING_PATH;
    }

    /* ------------------ STEP 3: Traverse to target node ------------------ */

    for (i = 1; i <= depth; i++) {
        Path_T oPrefix = Path_prefix(oTargetPath, i);
        if (oPrefix == NULL) {
            Path_free(oTargetPath);
            return MEMORY_ERROR;
        }

        boolean found = FALSE;
        size_t numChildren = Node_getNumChildren(oCurr);

        for (size_t j = 0; j < numChildren; j++) {
            Node_T child;
            if (Node_getChild(oCurr, j, &child) == SUCCESS &&
                Path_comparePath(Node_getPath(child), oPrefix) == 0) {
                oNext = child;
                found = TRUE;
                break;
            }
        }

        Path_free(oPrefix);

        if (!found) {
            Path_free(oTargetPath);
            return NO_SUCH_PATH;
        }

        oCurr = oNext;  // Move deeper
    }

    Path_free(oTargetPath);

    /* ------------------ STEP 4: Update output values based on type ------------------ */

    if (Node_getType(oCurr) == FT_FILE) {
        if (pbIsFile != NULL) *pbIsFile = TRUE;
        if (pulSize != NULL) {
            const char *contents = Node_getContents(oCurr);
            if (contents != NULL) {
                *pulSize = strlen(contents);
            } else {
                *pulSize = 0;
            }
        }
    } else {
        if (pbIsFile != NULL) *pbIsFile = FALSE;
        // *pulSize remains unchanged
    }

    return SUCCESS;
}

/*
  Sets the FT data structure to an initialized state.
  The data structure is initially empty.
  Returns INITIALIZATION_ERROR if already initialized,
  and SUCCESS otherwise.
*/
int FT_init(void) {
    /* If already initialized, return error */
    if (bIsInitialized) {
        return INITIALIZATION_ERROR;
    }

    /* Mark as initialized and reset root */
    bIsInitialized = TRUE;
    oRoot = NULL;

    return SUCCESS;
}

/*
  Removes all contents of the data structure and
  returns it to an uninitialized state.
  Returns INITIALIZATION_ERROR if not already initialized,
  and SUCCESS otherwise.
*/
int FT_destroy(void) {
    /* Check if the FT has been initialized */
    if (!bIsInitialized) {
        return INITIALIZATION_ERROR;
    }

    /* If there's a root node, free the entire tree */
    if (oRoot != NULL) {
        Node_free(oRoot);
        oRoot = NULL;
    }

    /* Mark the FT as uninitialized */
    bIsInitialized = FALSE;

    return SUCCESS;
}

/*
  Returns a string representation of the
  data structure, or NULL if the structure is
  not initialized or there is an allocation error.

  The representation is depth-first with files
  before directories at any given level, and nodes
  of the same type ordered lexicographically.

  Allocates memory for the returned string,
  which is then owned by client!
*/

static int compareNodes(const void *p1, const void *p2) {
    /* Comparison function for qsort to sort nodes lexicographically by path */
    const Node_T *node1 = (const Node_T *)p1;
    const Node_T *node2 = (const Node_T *)p2;
    return Node_compare(*node1, *node2);
}

/* Helper: recursively builds string representation (depth-first traversal) */
static boolean FT_traverseToString(Node_T oNode, DynArray_T oLines) {
    char *s = Node_toString(oNode);
    if (s == NULL) return FALSE;

    if (!DynArray_add(oLines, s)) {
        free(s);
        return FALSE;
    }

    /* Get children and separate into file and dir lists */
    size_t numChildren = Node_getNumChildren(oNode);
    DynArray_T fileChildren = DynArray_new(0);
    DynArray_T dirChildren = DynArray_new(0);

    if (fileChildren == NULL || dirChildren == NULL) {
        DynArray_free(fileChildren);
        DynArray_free(dirChildren);
        return FALSE;
    }

    for (size_t i = 0; i < numChildren; i++) {
        Node_T child;
        if (Node_getChild(oNode, i, &child) == SUCCESS) {
            if (Node_getType(child) == FT_FILE) {
                DynArray_add(fileChildren, child);
            } else {
                DynArray_add(dirChildren, child);
            }
        }
    }

    /* Sort each list lexicographically */
    qsort(DynArray_get(fileChildren), DynArray_getLength(fileChildren),
          sizeof(Node_T), compareNodes);

    qsort(DynArray_get(dirChildren), DynArray_getLength(dirChildren),
          sizeof(Node_T), compareNodes);

    /* Recursively process files first, then directories */
    for (size_t i = 0; i < DynArray_getLength(fileChildren); i++) {
        Node_T child = DynArray_get(fileChildren)[i];
        if (!FT_traverseToString(child, oLines)) {
            DynArray_free(fileChildren);
            DynArray_free(dirChildren);
            return FALSE;
        }
    }

    for (size_t i = 0; i < DynArray_getLength(dirChildren); i++) {
        Node_T child = DynArray_get(dirChildren)[i];
        if (!FT_traverseToString(child, oLines)) {
            DynArray_free(fileChildren);
            DynArray_free(dirChildren);
            return FALSE;
        }
    }

    DynArray_free(fileChildren);
    DynArray_free(dirChildren);
    return TRUE;
}

char *FT_toString(void) {
    if (!bIsInitialized || oRoot == NULL)
        return NULL;

    DynArray_T oLines = DynArray_new(0);
    if (oLines == NULL)
        return NULL;

    if (!FT_traverseToString(oRoot, oLines)) {
        /* Error occurred — cleanup */
        size_t len = DynArray_getLength(oLines);
        for (size_t i = 0; i < len; i++) {
            char *line = DynArray_get(oLines)[i];
            free(line);
        }
        DynArray_free(oLines);
        return NULL;
    }

    /* Compute total length of final string (including newlines + null) */
    size_t totalLength = 1; // for '\0'
    size_t numLines = DynArray_getLength(oLines);
    for (size_t i = 0; i < numLines; i++) {
        char *line = DynArray_get(oLines)[i];
        totalLength += strlen(line) + 1; // +1 for newline
    }

    char *result = malloc(totalLength);
    if (result == NULL) {
        for (size_t i = 0; i < numLines; i++)
            free(DynArray_get(oLines)[i]);
        DynArray_free(oLines);
        return NULL;
    }

    /* Build the result string */
    result[0] = '\0';
    for (size_t i = 0; i < numLines; i++) {
        strcat(result, DynArray_get(oLines)[i]);
        strcat(result, "\n");
        free(DynArray_get(oLines)[i]);  // Clean up individual lines
    }

    DynArray_free(oLines);
    return result;
}