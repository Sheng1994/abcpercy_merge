#include "map/if/if.h"

int main() {
    // Step 1: Allocate a new vector with initial capacity of 10
    Vec_Ptr_t *vIntPtrs = Vec_PtrAlloc(10);
    printf("Step 1: Initialized vector with capacity 10\n");

    // Step 2: Insert pointers to integers into the vector
    int *num1 = (int *)malloc(sizeof(int));
    int *num2 = (int *)malloc(sizeof(int));
    int *num3 = (int *)malloc(sizeof(int));
    *num1 = 10; *num2 = 20; *num3 = 10;  // Duplicate value for testing

    Vec_PtrPush(vIntPtrs, num1);
    Vec_PtrPush(vIntPtrs, num2);
    Vec_PtrPush(vIntPtrs, num3);  // Add duplicate intentionally

    // Try copy the Vec
    Vec_Ptr_t *vIntPtrsCopy = Vec_PtrAlloc(10);
    Vec_PtrCopy( vIntPtrsCopy, vIntPtrs );
    printf("    Copy vIntPtrs to vIntPtrsCopy\n");

    // check 2 vec is same or not
    // only 2 vec have the totally same mem address are same
    // they can be different though they have the same value

    int comp_result = Vec_PtrEqual( vIntPtrsCopy, vIntPtrs );
    printf("    Compare result (0 for diff, 1 for same): %d\n", comp_result);

    // Try find some number
    // can only find the number already in the container
    int find_result = Vec_PtrFind( vIntPtrs, num2);
    printf("    Num2 index in the Vec = %d\n", find_result);

    printf("Step 2: Added integers 10, 20, and 10 to the vector\n");
    printf("    Address stored in num1: %p\n", (void *)num1);
    printf("    Address stored in num2: %p\n", (void *)num2);
    printf("    Address stored in num3: %p\n", (void *)num3);

    // Step 3: Print the vector's content
    int i, *pValue;
    printf("Step 3: Vector content:\n");
    Vec_PtrForEachEntry(int*, vIntPtrs, pValue, i) {
        printf("    Entry %d: %d\n", i, *pValue);
    }

    // Step 4: Clean up memory
    printf("Step 4: Freeing allocated memory...\n");
    Vec_PtrForEachEntry(int*, vIntPtrs, pValue, i) {
        free(pValue);  // Free each allocated integer
    }
    Vec_PtrFree(vIntPtrs);  // Free the vector itself

}
