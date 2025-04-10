/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"

boolean CheckerDT_Node_isValid(Node_T oNNode) {
   Node_T oNParent;
   Path_T oPNPath;
   Path_T oPPPath;

   size_t ulNumChildren;
   size_t i, j;

   Node_T oneNode;
   Node_T twoNode;

   int cmp;

   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }

   oPNPath = Node_getPath(oNNode);
   if(oPNPath == NULL) {
      fprintf(stderr, "A node's path is NULL\n");
      return FALSE;
   }

   oNParent = Node_getParent(oNNode);
   if(oNParent != NULL) {
      oPPPath = Node_getPath(oNParent);
      if(oPPPath == NULL) {
         fprintf(stderr, "Parent node's path is NULL\n");
         return FALSE;
      }

      if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
         Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }
   }

   /* Check children list */
   ulNumChildren = Node_getNumChildren(oNNode);
   for(i = 0; i < ulNumChildren; i++) {
      Node_T oNChild = NULL;
      int iStatus = Node_getChild(oNNode, i, &oNChild);
      if(iStatus != SUCCESS || oNChild == NULL) {
         fprintf(stderr, "Child %lu of node %s is NULL or cannot be fetched\n",
                 (unsigned long)i, Path_getPathname(oPNPath));
         return FALSE;
      }

      if(Node_getParent(oNChild) != oNNode) {
         fprintf(stderr, "Child's parent pointer does not match expected parent: %s\n",
                 Path_getPathname(Node_getPath(oNChild)));
         return FALSE;
      }
   }

   /* Check for duplicate children */
   for(i = 0; i < ulNumChildren; i++) {
      Node_T oNChild1 = NULL;
      Node_getChild(oNNode, i, &oNChild1);
      for(j = i + 1; j < ulNumChildren; j++) {
         Node_T oNChild2 = NULL;
         Node_getChild(oNNode, j, &oNChild2);
         if(Path_comparePath(Node_getPath(oNChild1),
                             Node_getPath(oNChild2)) == 0) {
            fprintf(stderr, "Duplicate child path found under node %s: %s\n",
                    Path_getPathname(oPNPath),
                    Path_getPathname(Node_getPath(oNChild1)));
            return FALSE;
         }
      }
   }

   /* Check for lexicographic children order */
   for (i = 0; i < ulNumChildren - 1; i++) {
      if (Node_getChild(oNNode, i, &oneNode) != SUCCESS || oneNode == NULL ||
          Node_getChild(oNNode, i + 1, &twoNode) != SUCCESS || twoNode == NULL) {
         fprintf(stderr, "Failed to retrieve children at indices %lu and %lu for lexicographic check.\n",
                 (unsigned long)i, (unsigned long)(i + 1));
         return FALSE;
      }

      if (Node_getPath(oneNode) == NULL || Node_getPath(twoNode) == NULL) {
         fprintf(stderr, "Child node path is NULL.\n");
         return FALSE;
      }

      cmp = Path_comparePath(Node_getPath(oneNode), Node_getPath(twoNode));
      if (cmp > 0){
         fprintf(stderr, "Children are not in lexicographic order: %s > %s\n",
                 Path_getPathname(Node_getPath(oneNode)),
                 Path_getPathname(Node_getPath(twoNode)));
         return FALSE;
      }
   }

   return TRUE;
}

static boolean CheckerDT_treeCheck(Node_T oNNode, size_t *pulCount) {
   size_t i;
   size_t ulNumChildren;
   assert (pulCount != NULL);

   if(oNNode != NULL) {
      if(!CheckerDT_Node_isValid(oNNode)) {
         return FALSE;
      }

      (*pulCount)++;

      ulNumChildren = Node_getNumChildren(oNNode);
      for(i = 0; i < ulNumChildren; i++) {
         Node_T oNChild = NULL;
         if(Node_getChild(oNNode, i, &oNChild) != SUCCESS || oNChild == NULL) {
            fprintf(stderr, "Failed to fetch child %lu in traversal\n",
                    (unsigned long)i);
            return FALSE;
         }

         if(!CheckerDT_treeCheck(oNChild, pulCount)) {
            return FALSE;
         }
      }
   }
   return TRUE;
}

boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {
   size_t ulTraverseCount = 0;

   if(!bIsInitialized) {
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }
      if(oNRoot != NULL) {
         fprintf(stderr, "Not initialized, but root is not NULL\n");
         return FALSE;
      }
      return TRUE;
   }

   if(ulCount == 0 && oNRoot != NULL) {
      fprintf(stderr, "Count is 0, but root is not NULL\n");
      return FALSE;
   }

   if(ulCount > 0 && oNRoot == NULL) {
      fprintf(stderr, "Count is positive, but root is NULL\n");
      return FALSE;
   }

   if(!CheckerDT_treeCheck(oNRoot, &ulTraverseCount)) {
      return FALSE;
   }

   if(ulTraverseCount != ulCount) {
      fprintf(stderr, "Node count mismatch: expected %lu, got %lu\n",
              (unsigned long)ulCount, (unsigned long)ulTraverseCount);
      return FALSE;
   }

   return TRUE;
}
