/**CFile****************************************************************

  FileName    [ifCut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Cut computation.]

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifCut.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include <base/abc/abc.h>

#include "if.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Check correctness of cuts.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_CutVerifyCut( If_Cut_t * pBase, If_Cut_t * pCut ) // check if pCut is contained in pBase
{
    int nSizeB = pBase->nLeaves;
    int nSizeC = pCut->nLeaves;
    int * pB = pBase->pLeaves;
    int * pC = pCut->pLeaves;
    int i, k;
    for ( i = 0; i < nSizeC; i++ )
    {
        for ( k = 0; k < nSizeB; k++ )
            if ( pC[i] == pB[k] )
                break;
        if ( k == nSizeB )
            return 0;
    }
    return 1;
}
int If_CutVerifyCuts( If_Set_t * pCutSet, int fOrdered )
{
    static int Count = 0;
    If_Cut_t * pCut0, * pCut1;
    int i, k, m, n, Value;
    assert( pCutSet->nCuts > 0 );
    for ( i = 0; i < pCutSet->nCuts; i++ )
    {
        pCut0 = pCutSet->ppCuts[i];
        assert( pCut0->uSign == If_ObjCutSignCompute(pCut0) );
        if ( fOrdered )
        {
            // check duplicates
            for ( m = 1; m < (int)pCut0->nLeaves; m++ )
                assert( pCut0->pLeaves[m-1] < pCut0->pLeaves[m] );
        }
        else
        {
            // check duplicates
            for ( m = 0; m < (int)pCut0->nLeaves; m++ )
            for ( n = m+1; n < (int)pCut0->nLeaves; n++ )
            assert( pCut0->pLeaves[m] != pCut0->pLeaves[n] );
        }
        // check pairs
        for ( k = 0; k < pCutSet->nCuts; k++ )
        {
            pCut1 = pCutSet->ppCuts[k];
            if ( pCut0 == pCut1 )
                continue;
            Count++;
            // check containments
            Value = If_CutVerifyCut( pCut0, pCut1 );
//            assert( Value == 0 );
            if ( Value )
            {
                assert( pCut0->uSign == If_ObjCutSignCompute(pCut0) );
                assert( pCut1->uSign == If_ObjCutSignCompute(pCut1) );
                If_CutPrint( pCut0 );
                If_CutPrint( pCut1 );
                assert( 0 );
            }
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pDom is contained in pCut.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_CutCheckDominance( If_Cut_t * pDom, If_Cut_t * pCut )
{
    int i, k;
    assert( pDom->nLeaves <= pCut->nLeaves );
    for ( i = 0; i < (int)pDom->nLeaves; i++ )
    {
        for ( k = 0; k < (int)pCut->nLeaves; k++ )
            if ( pDom->pLeaves[i] == pCut->pLeaves[k] )
                break;
        if ( k == (int)pCut->nLeaves ) // node i in pDom is not contained in pCut
            return 0;
    }
    // every node in pDom is contained in pCut
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the cut is contained.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutFilter( If_Set_t * pCutSet, If_Cut_t * pCut, int fSaveCut0 )
{
    If_Cut_t * pTemp;
    int i, k;
    assert( pCutSet->ppCuts[pCutSet->nCuts] == pCut );
    for ( i = 0; i < pCutSet->nCuts; i++ )
    {
        pTemp = pCutSet->ppCuts[i];
        if ( pTemp->nLeaves > pCut->nLeaves )
        {
            // do not fiter the first cut
            if ( i == 0 && ((pCutSet->nCuts > 1 && pCutSet->ppCuts[1]->fUseless) || (fSaveCut0 && pCutSet->nCuts == 1)) )
                continue;
            // skip the non-contained cuts
            if ( (pTemp->uSign & pCut->uSign) != pCut->uSign )
                continue;
            // check containment seriously
            if ( If_CutCheckDominance( pCut, pTemp ) )
            {
//                p->ppCuts[i] = p->ppCuts[p->nCuts-1];
//                p->ppCuts[p->nCuts-1] = pTemp;
//                p->nCuts--;
//                i--;
                // remove contained cut
                for ( k = i; k < pCutSet->nCuts; k++ )
                    pCutSet->ppCuts[k] = pCutSet->ppCuts[k+1];
                pCutSet->ppCuts[pCutSet->nCuts] = pTemp;
                pCutSet->nCuts--;
                i--;
            }
         }
        else
        {
            // skip the non-contained cuts
            if ( (pTemp->uSign & pCut->uSign) != pTemp->uSign )
                continue;
            // check containment seriously
            if ( If_CutCheckDominance( pTemp, pCut ) )
                return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutMergeOrdered_( If_Man_t * p, If_Cut_t * pC0, If_Cut_t * pC1, If_Cut_t * pC )
{
    int nSizeC0 = pC0->nLeaves;
    int nSizeC1 = pC1->nLeaves;
    int nLimit  = pC0->nLimit;
    int i, k, c, s;

    // both cuts are the largest
    if ( nSizeC0 == nLimit && nSizeC1 == nLimit )
    {
        for ( i = 0; i < nSizeC0; i++ )
        {
            if ( pC0->pLeaves[i] != pC1->pLeaves[i] )
                return 0;
            p->pPerm[0][i] = p->pPerm[1][i] = p->pPerm[2][i] = i;
            pC->pLeaves[i] = pC0->pLeaves[i];
        }
        pC->nLeaves = nLimit;
        pC->uSign = pC0->uSign | pC1->uSign;
        p->uSharedMask = Abc_InfoMask( nLimit );
        return 1;
    }

    // compare two cuts with different numbers
    i = k = c = s = 0;
    p->uSharedMask = 0;
    if ( nSizeC0 == 0 ) goto FlushCut1;
    if ( nSizeC1 == 0 ) goto FlushCut0;
    while ( 1 )
    {
        if ( c == nLimit ) return 0;
        if ( pC0->pLeaves[i] < pC1->pLeaves[k] )
        {
            p->pPerm[0][i] = c;
            pC->pLeaves[c++] = pC0->pLeaves[i++];
            if ( i == nSizeC0 ) goto FlushCut1;
        }
        else if ( pC0->pLeaves[i] > pC1->pLeaves[k] )
        {
            p->pPerm[1][k] = c;
            pC->pLeaves[c++] = pC1->pLeaves[k++];
            if ( k == nSizeC1 ) goto FlushCut0;
        }
        else
        {
            p->uSharedMask |= (1 << c);
            p->pPerm[0][i] = p->pPerm[1][k] = p->pPerm[2][s++] = c;
            pC->pLeaves[c++] = pC0->pLeaves[i++]; k++;
            if ( i == nSizeC0 ) goto FlushCut1;
            if ( k == nSizeC1 ) goto FlushCut0;
        }
    }

FlushCut0:
    if ( c + nSizeC0 > nLimit + i ) return 0;
    while ( i < nSizeC0 )
    {
        p->pPerm[0][i] = c;
        pC->pLeaves[c++] = pC0->pLeaves[i++];
    }
    pC->nLeaves = c;
    pC->uSign = pC0->uSign | pC1->uSign;
    assert( c > 0 );
    return 1;

FlushCut1:
    if ( c + nSizeC1 > nLimit + k ) return 0;
    while ( k < nSizeC1 )
    {
        p->pPerm[1][k] = c;
        pC->pLeaves[c++] = pC1->pLeaves[k++];
    }
    pC->nLeaves = c;
    pC->uSign = pC0->uSign | pC1->uSign;
    assert( c > 0 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutMergeOrdered( If_Man_t * p, If_Cut_t * pC0, If_Cut_t * pC1, If_Cut_t * pC )
{
    int nSizeC0 = pC0->nLeaves;
    int nSizeC1 = pC1->nLeaves;
    int nLimit  = pC0->nLimit;
    int i, k, c, s;

    // Both cuts are the largest
    if ( nSizeC0 == nLimit && nSizeC1 == nLimit )
    {
        for ( i = 0; i < nSizeC0; i++ )
        {
            if ( pC0->pLeaves[i] != pC1->pLeaves[i] )
                return 0;
            pC->pLeaves[i] = pC0->pLeaves[i];
        }
        pC->nLeaves = nLimit;
        pC->uSign = pC0->uSign | pC1->uSign;

        // Vec_PtrClear(pC->vNodesInCut);
        // Vec_PtrAppend(pC->vNodesInCut, pC0->vNodesInCut);
        // Vec_PtrAppend(pC->vNodesInCut, pC1->vNodesInCut);
        return 1;
    }

    // Compare two cuts with different numbers
    i = k = c = s = 0;
    if ( nSizeC0 == 0 ) goto FlushCut1;
    if ( nSizeC1 == 0 ) goto FlushCut0;
    while ( 1 )
    {
        if ( c == nLimit ) return 0;

        if ( pC0->pLeaves[i] < pC1->pLeaves[k] )
        {
            pC->pLeaves[c++] = pC0->pLeaves[i++];
            if ( i == nSizeC0 ) goto FlushCut1;
        }
        else if ( pC0->pLeaves[i] > pC1->pLeaves[k] )
        {
            pC->pLeaves[c++] = pC1->pLeaves[k++];
            if ( k == nSizeC1 ) goto FlushCut0;
        }
        else
        {
            pC->pLeaves[c++] = pC0->pLeaves[i++]; k++;
            if ( i == nSizeC0 ) goto FlushCut1;
            if ( k == nSizeC1 ) goto FlushCut0;
        }
    }

FlushCut0:
    if ( c + nSizeC0 > nLimit + i ) return 0;
    while ( i < nSizeC0 )
        pC->pLeaves[c++] = pC0->pLeaves[i++];
    pC->nLeaves = c;
    pC->uSign = pC0->uSign | pC1->uSign;
    // goto MergeNodes;

FlushCut1:
    if ( c + nSizeC1 > nLimit + k ) return 0;
    while ( k < nSizeC1 )
        pC->pLeaves[c++] = pC1->pLeaves[k++];
    pC->nLeaves = c;
    pC->uSign = pC0->uSign | pC1->uSign;
    return 1;
    // goto MergeNodes;

// MergeNodes:
//     // Merge vNodesInCut
//     Vec_PtrClear(pC->vNodesInCut);
//     Vec_PtrAppend(pC->vNodesInCut, pC0->vNodesInCut);
//     Vec_PtrAppend(pC->vNodesInCut, pC1->vNodesInCut);
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutMerge( If_Man_t * p, If_Cut_t * pCut0, If_Cut_t * pCut1, If_Cut_t * pCut )
{
    int nLutSize = pCut0->nLimit;
    int nSize0 = pCut0->nLeaves;
    int nSize1 = pCut1->nLeaves;
    int * pC0 = pCut0->pLeaves;
    int * pC1 = pCut1->pLeaves;
    int * pC = pCut->pLeaves;
    int i, k, c;
    // compare two cuts with different numbers
    c = nSize0;
    for ( i = 0; i < nSize1; i++ )
    {
        for ( k = 0; k < nSize0; k++ )
            if ( pC1[i] == pC0[k] )
                break;
        if ( k < nSize0 )
        {
            p->pPerm[1][i] = k;
            continue;
        }
        if ( c == nLutSize )
            return 0;
        p->pPerm[1][i] = c;
        pC[c++] = pC1[i];
    }
    for ( i = 0; i < nSize0; i++ )
        pC[i] = pC0[i];
    pCut->nLeaves = c;
    pCut->uSign = pCut0->uSign | pCut1->uSign;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutCompareDelay( If_Man_t * p, If_Cut_t ** ppC0, If_Cut_t ** ppC1 )
{
    If_Cut_t * pC0 = *ppC0;
    If_Cut_t * pC1 = *ppC1;
    if ( pC0->Delay < pC1->Delay - p->fEpsilon )
        return -1;
    if ( pC0->Delay > pC1->Delay + p->fEpsilon )
        return 1;
    if ( pC0->nLeaves < pC1->nLeaves )
        return -1;
    if ( pC0->nLeaves > pC1->nLeaves )
        return 1;
    if ( pC0->Area < pC1->Area - p->fEpsilon )
        return -1;
    if ( pC0->Area > pC1->Area + p->fEpsilon )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutCompareDelayOld( If_Man_t * p, If_Cut_t ** ppC0, If_Cut_t ** ppC1 )
{
    If_Cut_t * pC0 = *ppC0;
    If_Cut_t * pC1 = *ppC1;
    if ( pC0->Delay < pC1->Delay - p->fEpsilon )
        return -1;
    if ( pC0->Delay > pC1->Delay + p->fEpsilon )
        return 1;
    if ( pC0->Area < pC1->Area - p->fEpsilon )
        return -1;
    if ( pC0->Area > pC1->Area + p->fEpsilon )
        return 1;
    if ( pC0->nLeaves < pC1->nLeaves )
        return -1;
    if ( pC0->nLeaves > pC1->nLeaves )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutCompareArea( If_Man_t * p, If_Cut_t ** ppC0, If_Cut_t ** ppC1 )
{
    If_Cut_t * pC0 = *ppC0;
    If_Cut_t * pC1 = *ppC1;
    if ( pC0->Area < pC1->Area - p->fEpsilon )
        return -1;
    if ( pC0->Area > pC1->Area + p->fEpsilon )
        return 1;
//    if ( pC0->AveRefs > pC1->AveRefs )
//        return -1;
//    if ( pC0->AveRefs < pC1->AveRefs )
//        return 1;
    if ( pC0->nLeaves < pC1->nLeaves )
        return -1;
    if ( pC0->nLeaves > pC1->nLeaves )
        return 1;
    if ( pC0->Delay < pC1->Delay - p->fEpsilon )
        return -1;
    if ( pC0->Delay > pC1->Delay + p->fEpsilon )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Comparison function for two cuts.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_ManSortCompare( If_Man_t * p, If_Cut_t * pC0, If_Cut_t * pC1 )
{
    if ( p->pPars->fPower )
    {
        if ( p->SortMode == 1 ) // area flow
        {
            if ( pC0->Area < pC1->Area - p->fEpsilon )
                return -1;
            if ( pC0->Area > pC1->Area + p->fEpsilon )
                return 1;
            //Abc_Print( 1,"area(%.2f, %.2f), power(%.2f, %.2f), edge(%.2f, %.2f)\n",
            //         pC0->Area, pC1->Area, pC0->Power, pC1->Power, pC0->Edge, pC1->Edge);
            if ( pC0->Power < pC1->Power - p->fEpsilon )
                return -1;
            if ( pC0->Power > pC1->Power + p->fEpsilon )
                return 1;
            if ( pC0->Edge < pC1->Edge - p->fEpsilon )
                return -1;
            if ( pC0->Edge > pC1->Edge + p->fEpsilon )
                return 1;
//            if ( pC0->AveRefs > pC1->AveRefs )
//                return -1;
//            if ( pC0->AveRefs < pC1->AveRefs )
//                return 1;
            if ( pC0->nLeaves < pC1->nLeaves )
                return -1;
            if ( pC0->nLeaves > pC1->nLeaves )
                return 1;
            if ( pC0->Delay < pC1->Delay - p->fEpsilon )
                return -1;
            if ( pC0->Delay > pC1->Delay + p->fEpsilon )
                return 1;
            return 0;
        }
        if ( p->SortMode == 0 ) // delay
        {
            if ( pC0->Delay < pC1->Delay - p->fEpsilon )
                return -1;
            if ( pC0->Delay > pC1->Delay + p->fEpsilon )
                return 1;
            if ( pC0->nLeaves < pC1->nLeaves )
                return -1;
            if ( pC0->nLeaves > pC1->nLeaves )
                return 1;
            if ( pC0->Area < pC1->Area - p->fEpsilon )
                return -1;
            if ( pC0->Area > pC1->Area + p->fEpsilon )
                return 1;
            if ( pC0->Power < pC1->Power - p->fEpsilon  )
                return -1;
            if ( pC0->Power > pC1->Power + p->fEpsilon  )
                return 1;
            if ( pC0->Edge < pC1->Edge - p->fEpsilon )
                return -1;
            if ( pC0->Edge > pC1->Edge + p->fEpsilon )
                return 1;
            return 0;
        }
        assert( p->SortMode == 2 ); // delay old, exact area
        if ( pC0->Delay < pC1->Delay - p->fEpsilon )
            return -1;
        if ( pC0->Delay > pC1->Delay + p->fEpsilon )
            return 1;
        if ( pC0->Power < pC1->Power - p->fEpsilon  )
            return -1;
        if ( pC0->Power > pC1->Power + p->fEpsilon  )
            return 1;
        if ( pC0->Edge < pC1->Edge - p->fEpsilon )
            return -1;
        if ( pC0->Edge > pC1->Edge + p->fEpsilon )
            return 1;
        if ( pC0->Area < pC1->Area - p->fEpsilon )
            return -1;
        if ( pC0->Area > pC1->Area + p->fEpsilon )
            return 1;
        if ( pC0->nLeaves < pC1->nLeaves )
            return -1;
        if ( pC0->nLeaves > pC1->nLeaves )
            return 1;
        return 0;
    }
    else  // regular
    {
        if ( p->SortMode == 1 ) // area
        {
            if ( pC0->Area < pC1->Area - p->fEpsilon )
                return -1;
            if ( pC0->Area > pC1->Area + p->fEpsilon )
                return 1;
            if ( pC0->Edge < pC1->Edge - p->fEpsilon )
                return -1;
            if ( pC0->Edge > pC1->Edge + p->fEpsilon )
                return 1;
            if ( pC0->Power < pC1->Power - p->fEpsilon )
                return -1;
            if ( pC0->Power > pC1->Power + p->fEpsilon )
                return 1;
//            if ( pC0->AveRefs > pC1->AveRefs )
//                return -1;
//            if ( pC0->AveRefs < pC1->AveRefs )
//                return 1;
            if ( pC0->nLeaves < pC1->nLeaves )
                return -1;
            if ( pC0->nLeaves > pC1->nLeaves )
                return 1;
            if ( pC0->fUseless < pC1->fUseless )
                return -1;
            if ( pC0->fUseless > pC1->fUseless )
                return 1;
            return 0;
        }
        if ( p->SortMode == 0 ) // delay
        {
            if ( pC0->Delay < pC1->Delay - p->fEpsilon )
                return -1;
            if ( pC0->Delay > pC1->Delay + p->fEpsilon )
                return 1;
            if ( pC0->nLeaves < pC1->nLeaves )
                return -1;
            if ( pC0->nLeaves > pC1->nLeaves )
                return 1;
            if ( pC0->Area < pC1->Area - p->fEpsilon )
                return -1;
            if ( pC0->Area > pC1->Area + p->fEpsilon )
                return 1;
            if ( pC0->Edge < pC1->Edge - p->fEpsilon )
                return -1;
            if ( pC0->Edge > pC1->Edge + p->fEpsilon )
                return 1;
            if ( pC0->Power < pC1->Power - p->fEpsilon )
                return -1;
            if ( pC0->Power > pC1->Power + p->fEpsilon )
                return 1;
            if ( pC0->fUseless < pC1->fUseless )
                return -1;
            if ( pC0->fUseless > pC1->fUseless )
                return 1;
            return 0;
        }
        assert( p->SortMode == 2 ); // delay old
        if ( pC0->Delay < pC1->Delay - p->fEpsilon )
            return -1;
        if ( pC0->Delay > pC1->Delay + p->fEpsilon )
            return 1;
        if ( pC0->fUseless < pC1->fUseless )
            return -1;
        if ( pC0->fUseless > pC1->fUseless )
            return 1;
        if ( pC0->Area < pC1->Area - p->fEpsilon )
            return -1;
        if ( pC0->Area > pC1->Area + p->fEpsilon )
            return 1;
        if ( pC0->Edge < pC1->Edge - p->fEpsilon )
            return -1;
        if ( pC0->Edge > pC1->Edge + p->fEpsilon )
            return 1;
        if ( pC0->Power < pC1->Power - p->fEpsilon )
            return -1;
        if ( pC0->Power > pC1->Power + p->fEpsilon )
            return 1;
        if ( pC0->nLeaves < pC1->nLeaves )
            return -1;
        if ( pC0->nLeaves > pC1->nLeaves )
            return 1;
        return 0;
    }
}

/**Function*************************************************************

  Synopsis    [Comparison function for two cuts.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_ManSortCompare_old( If_Man_t * p, If_Cut_t * pC0, If_Cut_t * pC1 )
{
    if ( p->SortMode == 1 ) // area
    {
        if ( pC0->Area < pC1->Area - p->fEpsilon )
            return -1;
        if ( pC0->Area > pC1->Area + p->fEpsilon )
            return 1;
//        if ( pC0->AveRefs > pC1->AveRefs )
//            return -1;
//        if ( pC0->AveRefs < pC1->AveRefs )
//            return 1;
        if ( pC0->nLeaves < pC1->nLeaves )
            return -1;
        if ( pC0->nLeaves > pC1->nLeaves )
            return 1;
        if ( pC0->Delay < pC1->Delay - p->fEpsilon )
            return -1;
        if ( pC0->Delay > pC1->Delay + p->fEpsilon )
            return 1;
        return 0;
    }
    if ( p->SortMode == 0 ) // delay
    {
        if ( pC0->Delay < pC1->Delay - p->fEpsilon )
            return -1;
        if ( pC0->Delay > pC1->Delay + p->fEpsilon )
            return 1;
        if ( pC0->nLeaves < pC1->nLeaves )
            return -1;
        if ( pC0->nLeaves > pC1->nLeaves )
            return 1;
        if ( pC0->Area < pC1->Area - p->fEpsilon )
            return -1;
        if ( pC0->Area > pC1->Area + p->fEpsilon )
            return 1;
        return 0;
    }
    assert( p->SortMode == 2 ); // delay old
    if ( pC0->Delay < pC1->Delay - p->fEpsilon )
        return -1;
    if ( pC0->Delay > pC1->Delay + p->fEpsilon )
        return 1;
    if ( pC0->Area < pC1->Area - p->fEpsilon )
        return -1;
    if ( pC0->Area > pC1->Area + p->fEpsilon )
        return 1;
    if ( pC0->nLeaves < pC1->nLeaves )
        return -1;
    if ( pC0->nLeaves > pC1->nLeaves )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs incremental sorting of cuts.]

  Description [Currently only the trivial sorting is implemented.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutSort( If_Man_t * p, If_Set_t * pCutSet, If_Cut_t * pCut )
{
//    int Counter = 0;
    int i;

    // the new cut is the last one
    assert( pCutSet->ppCuts[pCutSet->nCuts] == pCut );
    assert( pCutSet->nCuts <= pCutSet->nCutsMax );

    // cut structure is empty
    if ( pCutSet->nCuts == 0 )
    {
        pCutSet->nCuts++;
        return;
    }

    if ( !pCut->fUseless &&
         (p->pPars->fUseDsd || p->pPars->pFuncCell2 || p->pPars->fUseBat ||
          p->pPars->pLutStruct || p->pPars->fUserRecLib || p->pPars->fUserSesLib || p->pPars->fUserLutDec || p->pPars->fUserLut2D ||
          p->pPars->fEnableCheck07 || p->pPars->fUseCofVars || p->pPars->fUseAndVars || p->pPars->fUse34Spec ||
          p->pPars->fUseDsdTune || p->pPars->fEnableCheck75 || p->pPars->fEnableCheck75u || p->pPars->fUseCheck1 || p->pPars->fUseCheck2) )
    {
        If_Cut_t * pFirst = pCutSet->ppCuts[0];
        if ( pFirst->fUseless || If_ManSortCompare(p, pFirst, pCut) == 1 )
        {
            pCutSet->ppCuts[0] = pCut;
            pCutSet->ppCuts[pCutSet->nCuts] = pFirst;
            If_CutSort( p, pCutSet, pFirst );
            return;
        }
    }

    // the cut will be added - find its place
    for ( i = pCutSet->nCuts-1; i >= 0; i-- )
    {
//        Counter++;
        if ( If_ManSortCompare( p, pCutSet->ppCuts[i], pCut ) <= 0 || (i == 0 && !pCutSet->ppCuts[0]->fUseless && pCut->fUseless) )
            break;
        pCutSet->ppCuts[i+1] = pCutSet->ppCuts[i];
        pCutSet->ppCuts[i] = pCut;
    }
//    Abc_Print( 1, "%d ", Counter );

    // update the number of cuts
    if ( pCutSet->nCuts < pCutSet->nCutsMax )
        pCutSet->nCuts++;
}

/**Function*************************************************************

  Synopsis    [Orders the leaves of the cut.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutOrder( If_Cut_t * pCut )
{
    int i, Temp, fChanges;
    do {
        fChanges = 0;
        for ( i = 0; i < (int)pCut->nLeaves - 1; i++ )
        {
            assert( pCut->pLeaves[i] != pCut->pLeaves[i+1] );
            if ( pCut->pLeaves[i] <= pCut->pLeaves[i+1] )
                continue;
            Temp = pCut->pLeaves[i];
            pCut->pLeaves[i] = pCut->pLeaves[i+1];
            pCut->pLeaves[i+1] = Temp;
            fChanges = 1;
        }
    } while ( fChanges );
}

/**Function*************************************************************

  Synopsis    [Checks correctness of the cut.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutCheck( If_Cut_t * pCut )
{
    int i;
    assert( pCut->nLeaves <= pCut->nLimit );
    if ( pCut->nLeaves < 2 )
        return 1;
    for ( i = 1; i < (int)pCut->nLeaves; i++ )
    {
        if ( pCut->pLeaves[i-1] >= pCut->pLeaves[i] )
        {
            Abc_Print( -1, "If_CutCheck(): Cut has wrong ordering of inputs.\n" );
            return 0;
        }
        assert( pCut->pLeaves[i-1] < pCut->pLeaves[i] );
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Prints one cut.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutPrint( If_Cut_t * pCut )
{
    unsigned i;
    Abc_Print( 1, "{" );
    for ( i = 0; i < pCut->nLeaves; i++ )
        Abc_Print( 1, " %s%d", If_CutLeafBit(pCut, i) ? "!":"", pCut->pLeaves[i] );
    Abc_Print( 1, " }\n" );
}

/**Function*************************************************************

  Synopsis    [Prints one cut.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutPrintTiming( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    unsigned i;
    Abc_Print( 1, "{" );
    If_CutForEachLeaf( p, pCut, pLeaf, i )
        Abc_Print( 1, " %d(%.2f/%.2f)", pLeaf->Id, If_ObjCutBest(pLeaf)->Delay, pLeaf->Required );
    Abc_Print( 1, " }\n" );
}

/**Function*************************************************************

  Synopsis    [Moves the cut over the latch.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutLift( If_Cut_t * pCut )
{
    unsigned i;
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        assert( (pCut->pLeaves[i] & 255) < 255 );
        pCut->pLeaves[i]++;
    }
}


/**Function*************************************************************

  Synopsis    [Computes area flow.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutAreaFlow( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    float Flow, AddOn;
    int i;
    Flow = If_CutLutArea(p, pCut);
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        if ( pLeaf->nRefs == 0 || If_ObjIsConst1(pLeaf) )
            AddOn = If_ObjCutBest(pLeaf)->Area;
        else
        {
            assert( pLeaf->EstRefs > p->fEpsilon );
            AddOn = If_ObjCutBest(pLeaf)->Area / pLeaf->EstRefs;
        }
        if ( Flow >= (float)1e32 || AddOn >= (float)1e32 )
            Flow = (float)1e32;
        else
        {
            Flow += AddOn;
            if ( Flow > (float)1e32 )
                 Flow = (float)1e32;
        }
    }
    return Flow;
}

/**Function*************************************************************

  Synopsis    [Computes area flow.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutEdgeFlow( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    float Flow, AddOn;
    int i;
    Flow = pCut->nLeaves;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        if ( pLeaf->nRefs == 0 || If_ObjIsConst1(pLeaf) )
            AddOn = If_ObjCutBest(pLeaf)->Edge;
        else
        {
            assert( pLeaf->EstRefs > p->fEpsilon );
            AddOn = If_ObjCutBest(pLeaf)->Edge / pLeaf->EstRefs;
        }
        if ( Flow >= (float)1e32 || AddOn >= (float)1e32 )
            Flow = (float)1e32;
        else
        {
            Flow += AddOn;
            if ( Flow > (float)1e32 )
                 Flow = (float)1e32;
        }
    }
    return Flow;
}

/**Function*************************************************************

  Synopsis    [Computes area flow.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutPowerFlow( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pRoot )
{
    If_Obj_t * pLeaf;
    float * pSwitching = (float *)p->vSwitching->pArray;
    float Power = 0;
    int i;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        Power += pSwitching[pLeaf->Id];
        if ( pLeaf->nRefs == 0 || If_ObjIsConst1(pLeaf) )
            Power += If_ObjCutBest(pLeaf)->Power;
        else
        {
            assert( pLeaf->EstRefs > p->fEpsilon );
            Power += If_ObjCutBest(pLeaf)->Power / pLeaf->EstRefs;
        }
    }
    return Power;
}

/**Function*************************************************************

  Synopsis    [Average number of references of the leaves.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutAverageRefs( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    int nRefsTotal, i;
    nRefsTotal = 0;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
        nRefsTotal += pLeaf->nRefs;
    return ((float)nRefsTotal)/pCut->nLeaves;
}


/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutAreaDeref( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    float Area;
    int i;
    Area = If_CutLutArea(p, pCut);
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        assert( pLeaf->nRefs > 0 );
        if ( --pLeaf->nRefs > 0 || !If_ObjIsAnd(pLeaf) )
            continue;
        Area += If_CutAreaDeref( p, If_ObjCutBest(pLeaf) );
    }
    return Area;
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutAreaRef( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    float Area;
    int i;
    Area = If_CutLutArea(p, pCut);
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        assert( pLeaf->nRefs >= 0 );
        if ( pLeaf->nRefs++ > 0 || !If_ObjIsAnd(pLeaf) )
            continue;
        Area += If_CutAreaRef( p, If_ObjCutBest(pLeaf) );
    }
    return Area;
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutAreaDerefed( If_Man_t * p, If_Cut_t * pCut )
{
    float aResult, aResult2;
    if ( pCut->nLeaves < 2 )
        return 0;
    aResult2 = If_CutAreaRef( p, pCut );
    aResult  = If_CutAreaDeref( p, pCut );
    assert( aResult > aResult2 - 3*p->fEpsilon );
    assert( aResult < aResult2 + 3*p->fEpsilon );
    return aResult;
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutAreaRefed( If_Man_t * p, If_Cut_t * pCut )
{
    float aResult, aResult2;
    if ( pCut->nLeaves < 2 )
        return 0;
    aResult2 = If_CutAreaDeref( p, pCut );
    aResult  = If_CutAreaRef( p, pCut );
//    assert( aResult > aResult2 - p->fEpsilon );
//    assert( aResult < aResult2 + p->fEpsilon );
    return aResult;
}


/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutEdgeDeref( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    float Edge;
    int i;
    Edge = pCut->nLeaves;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        assert( pLeaf->nRefs > 0 );
        if ( --pLeaf->nRefs > 0 || !If_ObjIsAnd(pLeaf) )
            continue;
        Edge += If_CutEdgeDeref( p, If_ObjCutBest(pLeaf) );
    }
    return Edge;
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutEdgeRef( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    float Edge;
    int i;
    Edge = pCut->nLeaves;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        assert( pLeaf->nRefs >= 0 );
        if ( pLeaf->nRefs++ > 0 || !If_ObjIsAnd(pLeaf) )
            continue;
        Edge += If_CutEdgeRef( p, If_ObjCutBest(pLeaf) );
    }
    return Edge;
}

/**Function*************************************************************

  Synopsis    [Computes edge of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutEdgeDerefed( If_Man_t * p, If_Cut_t * pCut )
{
    float aResult, aResult2;
    if ( pCut->nLeaves < 2 )
        return pCut->nLeaves;
    aResult2 = If_CutEdgeRef( p, pCut );
    aResult  = If_CutEdgeDeref( p, pCut );
//    assert( aResult > aResult2 - 3*p->fEpsilon );
//    assert( aResult < aResult2 + 3*p->fEpsilon );
    return aResult;
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutEdgeRefed( If_Man_t * p, If_Cut_t * pCut )
{
    float aResult, aResult2;
    if ( pCut->nLeaves < 2 )
        return pCut->nLeaves;
    aResult2 = If_CutEdgeDeref( p, pCut );
    aResult  = If_CutEdgeRef( p, pCut );
//    assert( aResult > aResult2 - p->fEpsilon );
//    assert( aResult < aResult2 + p->fEpsilon );
    return aResult;
}


/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutPowerDeref( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pRoot )
{
    If_Obj_t * pLeaf;
    float * pSwitching = (float *)p->vSwitching->pArray;
    float Power = 0;
    int i;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        Power += pSwitching[pLeaf->Id];
        assert( pLeaf->nRefs > 0 );
        if ( --pLeaf->nRefs > 0 || !If_ObjIsAnd(pLeaf) )
            continue;
        Power += If_CutPowerDeref( p, If_ObjCutBest(pLeaf), pRoot );
    }
    return Power;
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutPowerRef( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pRoot )
{
    If_Obj_t * pLeaf;
    float * pSwitching = (float *)p->vSwitching->pArray;
    float Power = 0;
    int i;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        Power += pSwitching[pLeaf->Id];
        assert( pLeaf->nRefs >= 0 );
        if ( pLeaf->nRefs++ > 0 || !If_ObjIsAnd(pLeaf) )
            continue;
        Power += If_CutPowerRef( p, If_ObjCutBest(pLeaf), pRoot );
    }
    return Power;
}

/**Function*************************************************************

  Synopsis    [Computes Power of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutPowerDerefed( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pRoot )
{
    float aResult, aResult2;
    if ( pCut->nLeaves < 2 )
        return 0;
    aResult2 = If_CutPowerRef( p, pCut, pRoot );
    aResult  = If_CutPowerDeref( p, pCut, pRoot );
    assert( aResult > aResult2 - p->fEpsilon );
    assert( aResult < aResult2 + p->fEpsilon );
    return aResult;
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_CutPowerRefed( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pRoot )
{
    float aResult, aResult2;
    if ( pCut->nLeaves < 2 )
        return 0;
    aResult2 = If_CutPowerDeref( p, pCut, pRoot );
    aResult  = If_CutPowerRef( p, pCut, pRoot );
    assert( aResult > aResult2 - p->fEpsilon );
    assert( aResult < aResult2 + p->fEpsilon );
    return aResult;
}

/**Function*************************************************************

  Synopsis    [Computes the cone of the cut in AIG with choices.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutGetCutMinLevel( If_Man_t * p, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf;
    int i, nMinLevel = IF_INFINITY;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
        nMinLevel = IF_MIN( nMinLevel, (int)pLeaf->Level );
    return nMinLevel;
}

/**Function*************************************************************

  Synopsis    [Computes the cone of the cut in AIG with choices.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutGetCone_rec( If_Man_t * p, If_Obj_t * pObj, If_Cut_t * pCut )
{
    If_Obj_t * pTemp;
    int i, RetValue;
    // check if the node is in the cut
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        if ( pCut->pLeaves[i] == pObj->Id )
            return 1;
        else if ( pCut->pLeaves[i] > pObj->Id )
            break;
    // return if we reached the boundary
    if ( If_ObjIsCi(pObj) )
        return 0;
    // check the choice node
    for ( pTemp = pObj; pTemp; pTemp = pTemp->pEquiv )
    {
        // check if the node itself is bound
        RetValue = If_CutGetCone_rec( p, If_ObjFanin0(pTemp), pCut );
        if ( RetValue )
            RetValue &= If_CutGetCone_rec( p, If_ObjFanin1(pTemp), pCut );
        if ( RetValue )
            return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Computes the cone of the cut in AIG with choices.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutGetCones( If_Man_t * p )
{
    If_Obj_t * pObj;
    int i, Counter = 0;
    abctime clk = Abc_Clock();
    If_ManForEachObj( p, pObj, i )
    {
        if ( If_ObjIsAnd(pObj) && pObj->nRefs )
        {
            Counter += !If_CutGetCone_rec( p, pObj, If_ObjCutBest(pObj) );
//            Abc_Print( 1, "%d ", If_CutGetCutMinLevel( p, If_ObjCutBest(pObj) ) );
        }
    }
    Abc_Print( 1, "Cound not find boundary for %d nodes.\n", Counter );
    Abc_PrintTime( 1, "Cones", Abc_Clock() - clk );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Computes the cone of the cut in AIG with choices.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_CutFoundFanins_rec( If_Obj_t * pObj, Vec_Int_t * vLeaves )
{
    if ( pObj->nRefs || If_ObjIsCi(pObj) )
    {
        Vec_IntPushUnique( vLeaves, pObj->Id );
        return;
    }
    If_CutFoundFanins_rec( If_ObjFanin0(pObj), vLeaves );
    If_CutFoundFanins_rec( If_ObjFanin1(pObj), vLeaves );
}

/**Function*************************************************************

  Synopsis    [Computes the cone of the cut in AIG with choices.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutCountTotalFanins( If_Man_t * p )
{
    If_Obj_t * pObj;
    Vec_Int_t * vLeaves;
    int i, nFaninsTotal = 0, Counter = 0;
    abctime clk = Abc_Clock();
    vLeaves = Vec_IntAlloc( 100 );
    If_ManForEachObj( p, pObj, i )
    {
        if ( If_ObjIsAnd(pObj) && pObj->nRefs )
        {
            nFaninsTotal += If_ObjCutBest(pObj)->nLeaves;
            Vec_IntClear( vLeaves );
            If_CutFoundFanins_rec( If_ObjFanin0(pObj), vLeaves );
            If_CutFoundFanins_rec( If_ObjFanin1(pObj), vLeaves );
            Counter += Vec_IntSize(vLeaves);
        }
    }
    Abc_Print( 1, "Total cut inputs = %d. Total fanins incremental = %d.\n", nFaninsTotal, Counter );
    Abc_PrintTime( 1, "Fanins", Abc_Clock() - clk );
    Vec_IntFree( vLeaves );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutFilter2_rec( If_Man_t * p, If_Obj_t * pObj, int LevelMin )
{
    char * pVisited = Vec_StrEntryP(p->vMarks, pObj->Id);
    if ( *pVisited )
        return *pVisited;
    Vec_IntPush( p->vVisited2, pObj->Id );
    if ( (int)pObj->Level <= LevelMin )
        return (*pVisited = 1);
    if ( If_CutFilter2_rec( p, pObj->pFanin0, LevelMin ) == 1 )
        return (*pVisited = 1);
    if ( If_CutFilter2_rec( p, pObj->pFanin1, LevelMin ) == 1 )
        return (*pVisited = 1);
    return (*pVisited = 2);
}
int If_CutFilter2( If_Man_t * p, If_Obj_t * pNode, If_Cut_t * pCut )
{
    If_Obj_t * pLeaf, * pTemp;  int i, Count = 0;
//    printf( "Considering node %d and cut {", pNode->Id );
//    If_CutForEachLeaf( p, pCut, pLeaf, i )
//        printf( " %d", pLeaf->Id );
//    printf( " }\n" );
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        int k, iObj, RetValue, nLevelMin = ABC_INFINITY;
        Vec_IntClear( p->vVisited2 );
        If_CutForEachLeaf( p, pCut, pTemp, k )
        {
            if ( pTemp == pLeaf )
                continue;
            nLevelMin = Abc_MinInt( nLevelMin, (int)pTemp->Level );
            assert( Vec_StrEntry(p->vMarks, pTemp->Id) == 0 );
            Vec_StrWriteEntry( p->vMarks, pTemp->Id, 2 );
            Vec_IntPush( p->vVisited2, pTemp->Id );
        }
        RetValue = If_CutFilter2_rec( p, pLeaf, nLevelMin );
        Vec_IntForEachEntry( p->vVisited2, iObj, k )
            Vec_StrWriteEntry( p->vMarks, iObj, 0 );
        if ( RetValue == 2 )
        {
            Count++;
            pCut->nLeaves--;
            for ( k = i; k < (int)pCut->nLeaves; k++ )
                pCut->pLeaves[k] = pCut->pLeaves[k+1];
            i--;
        }
    }
    //if ( Count )
    //    printf( "%d", Count );
    return 0;
}

/*****************************user define**********************************/
If_Obj_t *deepCopyIfObj(const If_Obj_t *pObj) {
    if (pObj == NULL) {
        return NULL;  // Return NULL if the input object is NULL
    }

    // Allocate memory for the new object
    If_Obj_t *NewpObj = (If_Obj_t *)malloc(sizeof(If_Obj_t));
    if (NewpObj == NULL) {
        return NULL; // Memory allocation failed
    }

    // Copy the basic fields (primitive data)
    *NewpObj = *pObj;

    // Deep copy vectors, handling potential allocation failure and ensuring cleanup
    NewpObj->vFanouts = (pObj->vFanouts) ? Vec_PtrDup(pObj->vFanouts) : NULL;
    if (pObj->vFanouts && NewpObj->vFanouts == NULL) {
        free(NewpObj);  // Cleanup allocated memory if deep copy failed
        return NULL;
    }

    NewpObj->vCutsWithNode = (pObj->vCutsWithNode) ? Vec_PtrDup(pObj->vCutsWithNode) : NULL;
    if (pObj->vCutsWithNode && NewpObj->vCutsWithNode == NULL) {
        free(NewpObj->vFanouts);  // Cleanup vFanouts if vCutsWithNode copy failed
        free(NewpObj);
        return NULL;
    }

    NewpObj->CutBest.vNodesInCut = (pObj->CutBest.vNodesInCut) ? Vec_PtrDup(pObj->CutBest.vNodesInCut) : NULL;
    if (pObj->CutBest.vNodesInCut && NewpObj->CutBest.vNodesInCut == NULL) {
        free(NewpObj->vFanouts);  // Cleanup vFanouts if vNodesInCut copy failed
        free(NewpObj->vCutsWithNode);  // Cleanup vCutsWithNode if vNodesInCut copy failed
        free(NewpObj);
        return NULL;
    }

    // Ensure pFanin0, pFanin1, pEquiv are copied as-is (no deep copy needed for these)
    NewpObj->pFanin0 = pObj->pFanin0;
    NewpObj->pFanin1 = pObj->pFanin1;
    NewpObj->pEquiv  = pObj->pEquiv;

    return NewpObj;
}


void freeIfObj(If_Obj_t *pObj) {
    if (pObj == NULL) {
        return;
    }

    // Free dynamically allocated vector pointers (assuming Vec_PtrFree is available)
    if (pObj->vFanouts) {
        Vec_PtrFree(pObj->vFanouts);
    }
    if (pObj->vCutsWithNode) {
        Vec_PtrFree(pObj->vCutsWithNode);
    }
    // if (pObj->vCutsWithLeave) {
    //     Vec_PtrFree(pObj->vCutsWithLeave);
    // }
    // if (pObj->vNearCut) {
    //     Vec_PtrFree(pObj->vNearCut);
    // }
    if (pObj->vKLCut) {
        Vec_PtrFree(pObj->vKLCut);
    }
    // Finally, free the main object
    free(pObj);
}


/**Function*************************************************************

  Synopsis    [DAG of each cut]

***********************************************************************/

void If_CutDAG(If_Man_t* p, If_Cut_t* pCut)
{
    // Create a new vector to hold the netDAG (2D array)
    // Vec_Ptr_t *netDAG = Vec_PtrAlloc(pCut->vNodesInCut->nSize);  // Size based on the number of nodes in the cut
    // Vec_Ptr_t *DagOP = Vec_PtrAlloc(pCut->vNodesInCut->nSize);
    pCut->netDAG = Vec_PtrAlloc(pCut->vNodesInCut->nSize);
    pCut->DagOP = Vec_PtrAlloc(pCut->vNodesInCut->nSize);
    // Iterate over the nodes in the cut
    for (int j = 0; j < pCut->vNodesInCut->nSize; j++) {
        // Create a new row with 3 columns
        Vec_Ptr_t *vRow = Vec_PtrAlloc(3);
        Vec_Ptr_t *vInv = Vec_PtrAlloc(3);
        // Get the node corresponding to the current entry in the cut
        If_Obj_t* pNode = (If_Obj_t*)Vec_PtrEntry(p->vObjs, *(int*)pCut->vNodesInCut->pArray[j]);
        // Allocate memory for the fanin and node IDs
        int *pNum1 = (int *)malloc(sizeof(int));
        int *pInv1 = (int *)malloc(sizeof(int));
        *pNum1 = (pNode->pFanin0 != NULL) ? pNode->pFanin0->Id : 0;
        *pInv1 = (pNode->pFanin0 != NULL) ? pNode->fCompl0 : 0;
        int *pNum2 = (int *)malloc(sizeof(int));
        int *pInv2 = (int *)malloc(sizeof(int));
        *pNum2 = (pNode->pFanin1 != NULL) ? pNode->pFanin1->Id : 0;
        *pInv2 = (pNode->pFanin1 != NULL) ? pNode->fCompl1 : 0;
        int *pNum3 = (int *)malloc(sizeof(int));
        int *pInv3 = (int *)malloc(sizeof(int));
        *pNum3 = pNode->Id; *pInv3 = pNode->fPhase;
        // Push the values into the row (vRow)
        Vec_PtrPush(vRow, pNum1); Vec_PtrPush(vInv, pInv1);
        Vec_PtrPush(vRow, pNum2); Vec_PtrPush(vInv, pInv2);
        Vec_PtrPush(vRow, pNum3); Vec_PtrPush(vInv, pInv3);
        // Add the row to the netDAG
        Vec_PtrPush(pCut->netDAG, vRow); Vec_PtrPush(pCut->DagOP, vInv);
        // Optional: Print the node index (for debugging)
        //printf("%d ", *(int*)pCut->vNodesInCut->pArray[j]);
    }
    //printf("\n");
    // for (int i = 0; i < Vec_PtrSize(pCut->netDAG); i++) {
    //     Vec_Ptr_t *vRow = (Vec_Ptr_t *)Vec_PtrEntry(pCut->netDAG, i);
    //     Vec_Ptr_t *vInv = (Vec_Ptr_t *)Vec_PtrEntry(pCut->DagOP, i);
    //     for (int j = 0; j < Vec_PtrSize(vRow); j++) {
    //         int *pNum = (int *)Vec_PtrEntry(vRow, j);
    //         int *pInv = (int *)Vec_PtrEntry(vInv, j);
    //         printf("netDAG[%d][%d] = %d with %d inv\n", i, j, *pNum, *pInv);
    //     }
    // }

    printf("\n");
    // Return the 2D array (netDAG)
    // return netDAG;
}

/**Function*************************************************************

  Synopsis    [initialize NodeInCuts]

***********************************************************************/
void If_ManInitializeNodeCuts(If_Man_t* pMan)
{
    If_Obj_t* pNode;
    int i;

    If_ManForEachNode(pMan, pNode, i)
    {
        // if vNodesInCut is not empty, free it
        if (pNode->CutBest.vNodesInCut)
            Vec_PtrFree(pNode->CutBest.vNodesInCut);
        // allocate memory for vNodesInCut 16 pointers
        pNode->CutBest.vNodesInCut = Vec_PtrAlloc(16);
        Vec_PtrPush(pNode->CutBest.vNodesInCut, &pNode->Id);
    }
}

/**Function*************************************************************

  Synopsis    [Merge NodesInCut]

***********************************************************************/
void If_CutNodesMerge(If_Cut_t* pCut0, If_Cut_t* pCut1, If_Cut_t* pCut)
{
    int i = 0,j = 0;
    for (i = 0; i < pCut0->vNodesInCut->nSize; i++) {
        for (j = 0; j < pCut1->vNodesInCut->nSize; j++) {
            int num0 = *(int *)pCut0->vNodesInCut->pArray[i];
            int num1 = *(int *)pCut1->vNodesInCut->pArray[j];
            int isleave0 = 0; int isleave1 = 0;
            // Check if num1/num2 belong to Leaves
            for (int k = 0; k < pCut->nLeaves; k++) {
                int leave_temp = pCut->pLeaves[k];
                if (num0 == leave_temp) {
                    isleave0 = 1;
                }
                if (num1 == leave_temp) {
                    isleave1 = 1;
                }
            }

            // If the numbers are the same and not already in vNodesInCut
            if (num0 == num1) {
                int alreadyExists = 0;
                for (int k = 0; k < pCut->vNodesInCut->nSize; k++) {
                    if (*(int *)pCut->vNodesInCut->pArray[k] == num0) {
                        alreadyExists = 1;
                        break;
                    }
                }
                if (!alreadyExists && isleave0 == 0) {
                    Vec_PtrPush(pCut->vNodesInCut, pCut0->vNodesInCut->pArray[i]);
                }
            } else {
                // Check for num0
                int alreadyExistsNum0 = 0;
                for (int k = 0; k < pCut->vNodesInCut->nSize; k++) {
                    if (*(int *)pCut->vNodesInCut->pArray[k] == num0) {
                        alreadyExistsNum0 = 1;
                        break;
                    }
                }
                if (!alreadyExistsNum0 && isleave0 == 0) {
                    Vec_PtrPush(pCut->vNodesInCut, pCut0->vNodesInCut->pArray[i]);
                }

                // Check for num1
                int alreadyExistsNum1 = 0;
                for (int k = 0; k < pCut->vNodesInCut->nSize; k++) {
                    if (*(int *)pCut->vNodesInCut->pArray[k] == num1) {
                        alreadyExistsNum1 = 1;
                        break;
                    }
                }
                if (!alreadyExistsNum1 && isleave1 == 0) {
                    Vec_PtrPush(pCut->vNodesInCut, pCut1->vNodesInCut->pArray[j]);
                }
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Find each node's fanouts]

***********************************************************************/
/*void If_CutFanoutVec(If_Man_t* p) {

    If_Obj_t * pObx;
    If_Obj_t * pOby;
    int x, y;

    // Ensure all objects have a fanouts vector initialized
    If_ManForEachObj(p, pOby, x) {
        if (pOby->vFanouts == NULL) {
            pOby->vFanouts = Vec_PtrAlloc(100); // Allocate once per object
        }
    }


    printf("The fanout process: ");
    If_ManForEachObj(p, pObx, x) {
        if (pObx == NULL) continue;
        int currNodeId = pObx->Id;
        printf("%d ", currNodeId);
        If_ManForEachObj(p, pOby, y) {
            if (pOby == NULL) continue;
            int possFanin0Id = (pOby->pFanin0 != NULL) ? pOby->pFanin0->Id : -1;
            int possFanin1Id = (pOby->pFanin1 != NULL) ? pOby->pFanin1->Id : -1;
            if (currNodeId == possFanin0Id || currNodeId == possFanin1Id) {
                Vec_PtrPush(pObx->vFanouts, &pOby->Id); // Push to the correct object's vector
            }
        }
    }

    // print the fanout node Id
    // int i;
    // If_ManForEachObj(p, pOby, x) {
    //     printf("The node %d has fanouts: ");
    //     if (pOby->vFanouts->nSize != 0) {
    //         for (i = 0; i < pOby->vFanouts->nSize; i++) {
    //             printf("%d ", *(int *)pOby->vFanouts->pArray[i]);
    //         }
    //     }
    //     printf("\n");
    // }
}*/

void If_CutFanoutVec(If_Man_t* p) {
    If_Obj_t* pOby;
    int x;

    // Initialize fanouts vector for each object
    If_ManForEachObj(p, pOby, x) {
        if (pOby->vFanouts == NULL) {
            pOby->vFanouts = Vec_PtrAlloc(4);
        }
    }

    If_ManForEachObj(p, pOby, x) {
        if (pOby->pFanin0 != NULL)
            Vec_PtrPushUnique(pOby->pFanin0->vFanouts, &pOby->Id);
        if (pOby->pFanin1 != NULL)
            Vec_PtrPushUnique(pOby->pFanin1->vFanouts, &pOby->Id);
    }
}

void If_AbcCutFanoutVec(If_Man_t* pIfMan, Abc_Ntk_t * pNtk) {
    Abc_Obj_t * pNode; int i;
    // Abc_NtkForEachNode(pNtkTemp, pNode1, i) pNode1->fMarker = false;
    Abc_NtkForEachObj(pNtk, pNode, i) {
        Vec_Int_t vFanouts = pNode->vFanouts;
        If_Obj_t *pObjIf = (If_Obj_t *) Vec_PtrEntry(pIfMan->vObjs, i);
        for (int j = 0; j <pNode->vFanouts.nSize; j++) {
            int nodetemp = vFanouts.pArray[j];
            If_Obj_t *CurrNode = (If_Obj_t *) Vec_PtrEntry(pIfMan->vObjs, nodetemp);
            if (pObjIf->vFanouts == NULL) {pObjIf->vFanouts = Vec_PtrAlloc(100);}
            Vec_PtrPushUnique(pObjIf->vFanouts, &CurrNode->Id);
        }
    }
    // print the fanout node Id

    If_Obj_t * pOby; int x;
    If_ManForEachObj(pIfMan, pOby, x) {
        printf("The node %d has fanouts: ", pOby->Id);
        if (pOby->vFanouts != NULL) {
            for (i = 0; i < pOby->vFanouts->nSize; i++) {
                printf("%d ", *(int *)pOby->vFanouts->pArray[i]);
            }
        }
        printf("\n");
    }
}

/**Function*************************************************************

  Synopsis    [Find cuts with node i]

***********************************************************************/
void If_CutsWithNode(If_Man_t* p) {
    int i;
    If_Obj_t * pObj;
    If_ManForEachNode(p, pObj, i) {
        pObj->vCutsWithNode = Vec_PtrAlloc(20);
        Vec_PtrPush(pObj->vCutsWithNode, &pObj->Id);
        If_Cut_t * pCut = &pObj->CutBest;
        for (int j=0; j<pCut->vNodesInCut->nSize;j++) {
            int nodetemp = *(int *)pCut->vNodesInCut->pArray[j];
            If_Obj_t *CurrNode = (If_Obj_t *) Vec_PtrEntry(p->vObjs, nodetemp);
            int alreadyExists = 0;
            for (int k = 0; k < CurrNode->vCutsWithNode->nSize; k++) {
                if (*(int *)CurrNode->vCutsWithNode->pArray[k] == pObj->Id) {
                    alreadyExists = 1;
                    break;
                }
            }
            if (!alreadyExists) {
                Vec_PtrPush(CurrNode->vCutsWithNode, &pObj->Id);
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Find cuts with node i as leaf]

***********************************************************************/
// void If_CutsWithLeaf(If_Man_t* p) {
//     If_Obj_t * pObj; int i;
//     If_ManForEachObj(p, pObj, i) {
//         pObj->vCutsWithLeave = Vec_PtrAlloc(20);
//         //Vec_PtrPush(pObj->vCutsWithNode, &pObj->Id);
//         If_Cut_t * pCut = &pObj->CutBest;
//         for (int j=0; j<pCut->nLeaves;j++) {
//             int nodetemp = pCut->pLeaves[j];
//             If_Obj_t *CurrNode = (If_Obj_t *) Vec_PtrEntry(p->vObjs, nodetemp);
//             int alreadyExists = 0;
//             if (CurrNode->vCutsWithLeave!=NULL) {
//                 for (int k = 0; k < CurrNode->vCutsWithLeave->nSize; k++) {
//                     if (*(int *)CurrNode->vCutsWithLeave->pArray[k] == pObj->Id) {
//                         alreadyExists = 1;
//                         break;
//                     }
//                 }
//             }
//             // if (!CurrNode->vCutsWithLeave) {
//             //     CurrNode->vCutsWithLeave = Vec_PtrAlloc(20);  // Allocate memory if not already allocated
//             // }
//             if (!alreadyExists) {
//                 Vec_PtrPush(CurrNode->vCutsWithLeave, &pObj->Id);
//             }
//         }
//     }
// }
/**Function*************************************************************

  Synopsis    [Find cuts with node i as leaf based on leaves]

***********************************************************************/
// void If_NearCutEnuLeaves(If_Man_t* p, int maxCuts) {
//
//     If_Obj_t * pObj; int i;
//     If_ManForEachNode(p, pObj, i) {
//         If_Cut_t *currentCut = &pObj->CutBest;
//         int *currentLeaves = pObj->CutBest.pLeaves;
//         int selectedCutIndex = 0;
//         int maxMatchingLeaves = 0;
//
//         // Dynamically allocate memory for storing top cuts and their counts
//         int *topCutIndices = (int *)malloc(maxCuts * sizeof(int));
//         int *topMatchingCounts = (int *)malloc(maxCuts * sizeof(int));
//
//         // Initialize topCutIndices and topMatchingCounts arrays
//         for (int index = 0; index < maxCuts; index++) {
//             topCutIndices[index] = -1;  // Initialize with invalid values
//             topMatchingCounts[index] = 0; // Initialize counts to 0
//         }
//         for (int leafIndex = 0; leafIndex < currentCut->nLeaves; leafIndex++) {
//             int currentNodeIndex = currentCut->pLeaves[leafIndex];
//             If_Obj_t *currentNode = (If_Obj_t *) Vec_PtrEntry(p->vObjs, currentNodeIndex);
//             for (int cutIndexInNode = 0; cutIndexInNode < currentNode->vCutsWithLeave->nSize; cutIndexInNode++) {
//                 int candidateCutIndex = *(int *)currentNode->vCutsWithLeave->pArray[cutIndexInNode];
//                 if (pObj->Id == candidateCutIndex) {
//                     continue;  // Skip the node itself
//                 }
//                 // Check if candidateCutIndex already exists in topCutIndices
//                 int isDuplicate = 0;
//                 for (int topCutIndex = 0; topCutIndex < maxCuts; topCutIndex++) {
//                     if (topCutIndices[topCutIndex] == candidateCutIndex) {
//                         isDuplicate = 1;  // Set flag if duplicate is found
//                         break;
//                     }
//                 }
//                 if (isDuplicate) {
//                     continue;  // Skip adding this candidateCutIndex if it's a duplicate
//                 }
//                 If_Obj_t *candidateCutNode = (If_Obj_t *) Vec_PtrEntry(p->vObjs, candidateCutIndex);
//                 int *candidateCutLeaves = candidateCutNode->CutBest.pLeaves;
//                 int matchingLeavesCount = 0;
//                 // Compare leaves between currentCut and candidateCutNode
//                 for (int currentLeafIndex = 0; currentLeafIndex < currentCut->nLeaves; currentLeafIndex++) {
//                     for (int candidateLeafIndex = 0; candidateLeafIndex < candidateCutNode->CutBest.nLeaves; candidateLeafIndex++) {
//                         if (currentLeaves[currentLeafIndex] == candidateCutLeaves[candidateLeafIndex]) {
//                             matchingLeavesCount++;
//                             break;
//                         }
//                     }
//                 }
//                 // Insert the current candidateCutIndex and matchingLeavesCount into the topCuts array if it's one of the top N
//                 for (int topCutIndex = 0; topCutIndex < maxCuts; topCutIndex++) {
//                     if (matchingLeavesCount > topMatchingCounts[topCutIndex]) {
//                         // Shift the elements down
//                         for (int shiftIndex = maxCuts - 1; shiftIndex > topCutIndex; shiftIndex--) {
//                             topMatchingCounts[shiftIndex] = topMatchingCounts[shiftIndex - 1];
//                             topCutIndices[shiftIndex] = topCutIndices[shiftIndex - 1];
//                         }
//                         topMatchingCounts[topCutIndex] = matchingLeavesCount;
//                         topCutIndices[topCutIndex] = candidateCutIndex;
//                         break;
//                     }
//                 }
//             }
//         }
//         // Store the top cuts in pObj->vNearCut
//         if (!pObj->vNearCut) {
//             pObj->vNearCut = Vec_PtrAlloc(maxCuts);  // Allocate memory if not already allocated
//         }
//         // Push the top cuts to the vector
//         for (int topCutIndex = 0; topCutIndex < maxCuts; topCutIndex++) {
//             if (topCutIndices[topCutIndex] > p->vCis->nSize) {  // Only add valid cuts
//                 Vec_PtrPush(pObj->vNearCut, &topCutIndices[topCutIndex]);
//                 printf("Selected Cut Index %d for Node %d with %d matching leaves is: %d\n",
//                        topCutIndex + 1, i, topMatchingCounts[topCutIndex], topCutIndices[topCutIndex]);
//             }
//         }
//         // Free dynamically allocated memory
//         free(topCutIndices);
//         free(topMatchingCounts);
//     }
// }

/**Function*************************************************************

  Synopsis    [Find cuts with node i as leaf based on fanin and fanout]

***********************************************************************/
// void If_NearCutEnuIOs( If_Man_t* p ) {
//     If_Obj_t * pObj; int i;
//     If_ManForEachNode(p, pObj, i) {
//         Vec_Ptr_t *vCutsFanin0 = pObj->pFanin0->vFanouts;
//         Vec_Ptr_t *vCutsFanin1 = pObj->pFanin1->vFanouts;
//         Vec_Ptr_t *vFanouts = pObj->vFanouts;
//         // Add elements from vCutsFanin0 to vNearCut
//         if (!pObj->vNearCut) {
//             // Allocate memory if not already allocated
//             pObj->vNearCut = Vec_PtrAlloc(vCutsFanin0->nSize+vCutsFanin1->nSize);
//         }
//         // node->fanin->fanout
//         for (int j = 0; j < vCutsFanin0->nSize; j++) {
//             Vec_PtrPush(pObj->vNearCut, vCutsFanin0->pArray[j]);
//         }
//         // Add elements from vCutsFanin1 to vNearCut if not already present
//         for (int j = 0; j < vCutsFanin1->nSize; j++) {
//             Vec_PtrPushUnique(pObj->vNearCut, vCutsFanin1->pArray[j]);
//         }
//         // node->fanout->fanin
//         for (int j = 0; j < vFanouts->nSize; j++) {
//             int nodetemp = *(int *)vFanouts->pArray[j];
//             If_Obj_t *CurrNode = (If_Obj_t *) Vec_PtrEntry(p->vObjs, nodetemp);
//             if (CurrNode->Type == 4) {
//                 Vec_PtrPushUnique(pObj->vNearCut, &CurrNode->pFanin0->Id);
//                 Vec_PtrPushUnique(pObj->vNearCut, &CurrNode->pFanin1->Id);
//             }
//         }
//     }
// }
/**Function******************user define********************************

  Synopsis    [Find cuts based on fanin/fanout recursive search]

***********************************************************************/
// int Vec_Ismemeber(Vec_Ptr_t *vNodesInCut,int numtemp) {
//     for (int j = 0; j < vNodesInCut->nSize; j++) {
//         int nodeindex = *(int *)vNodesInCut->pArray[j];
//         if (nodeindex == numtemp) {
//             return 1;
//             break;
//         }
//     }
//     return 0;
// }

Vec_Ptr_t *FaninCount(If_Man_t *p, Vec_Ptr_t *vNodesInCut) {
    int faninCount = 0;
    Vec_Ptr_t *faninlist = Vec_PtrAlloc(0);
    Vec_Ptr_t *fanins = Vec_PtrAlloc(0);
    for (int i = 0; i < vNodesInCut->nSize; i++) {
        int nodetemp = *(int *)vNodesInCut->pArray[i];
        If_Obj_t *currentNode = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
        if (currentNode->pFanin0 != NULL)
            Vec_PtrPushUnique(faninlist, &currentNode->pFanin0->Id);
        if (currentNode->pFanin1 != NULL)
            Vec_PtrPushUnique(faninlist, &currentNode->pFanin1->Id);
    }
    for (int i = 0; i < faninlist->nSize; i++) {
        int nodetemp = *(int *)faninlist->pArray[i];
        If_Obj_t *currentNode = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
        if (!Vec_Ismemeber(vNodesInCut,nodetemp)) {
            Vec_PtrPushUnique(fanins, &currentNode->Id);
            faninCount = faninCount + 1;
        }
    }
    Vec_PtrFree(faninlist);
    return fanins;
}

Vec_Ptr_t *FanoutCount(If_Man_t *p, Vec_Ptr_t *vNodesInCut) {
    int fanoutCount = 0;
    Vec_Ptr_t *faninlist = Vec_PtrAlloc(0);
    Vec_Ptr_t *fanouts = Vec_PtrAlloc(0);
    for (int i = 0; i < vNodesInCut->nSize; i++) {
        int nodetemp = *(int *)vNodesInCut->pArray[i];
        If_Obj_t *currentNode = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
        if (currentNode->Type == 4) {
            Vec_PtrPushUnique(faninlist, &currentNode->pFanin0->Id);
            Vec_PtrPushUnique(faninlist, &currentNode->pFanin1->Id);
        }
    }
    for (int i = 0; i < vNodesInCut->nSize; i++) {
        int nodetemp = *(int *)vNodesInCut->pArray[i];
        If_Obj_t *currentNode = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
        if (!Vec_Ismemeber(faninlist,nodetemp)) {
            Vec_PtrPushUnique(fanouts, &currentNode->Id);
            fanoutCount = fanoutCount + 1;
        }
    }
    Vec_PtrFree(faninlist);
    return fanouts;
}

void If_ManDevRec(If_Obj_t *pObj, If_Obj_t *curr_obj, Vec_Ptr_t *tempLeaves) {
    if (Vec_Ismemeber(pObj->vBestKLFanins, curr_obj->Id)) {
        Vec_PtrPushUnique(tempLeaves, &curr_obj->Id);
    } else if (curr_obj->pFanin0 != NULL && curr_obj->pFanin1 != NULL) {
        If_ManDevRec(pObj, curr_obj->pFanin0, tempLeaves);
        If_ManDevRec(pObj, curr_obj->pFanin1, tempLeaves);
    }
}

void If_ManLeafDev(If_Man_t *p, If_Obj_t *pObj) {
    Vec_Ptr_t *tempLeaves = Vec_PtrAlloc(0);
    If_ManDevRec(pObj, pObj, tempLeaves);
    Vec_PtrFreeP(&pObj->vBestKLNonFanins);
    pObj->vBestKLNonFanins = Vec_PtrAlloc(0);
    for (int k = 0; k < pObj->vBestKLFanins->nSize; k++) {
        int *pNum = (int *)Vec_PtrEntry(pObj->vBestKLFanins, k);
        if (!Vec_Ismemeber(tempLeaves, *pNum)) {
            Vec_PtrPushUnique(pObj->vBestKLNonFanins, pNum);
        }
    }
    Vec_PtrClear(pObj->vBestKLFanins);
    Vec_PtrCopy(pObj->vBestKLFanins,  tempLeaves);
    Vec_PtrFree(tempLeaves);
}

int vKLCutRepeated(If_Obj_t *pObj, Vec_Ptr_t *vNodesInCut) {
    int isrepeated = 1;  // Initialize once
    for (int j = 0; j < Vec_PtrSize(pObj->vKLCut); j++) {
        Vec_Ptr_t *vRow = (Vec_Ptr_t *)Vec_PtrEntry(pObj->vKLCut, j);
        if (vRow->nSize != vNodesInCut->nSize) {
            continue;
        } else {
            isrepeated = 1;  // Reset for each row comparison
            for (int k = 0; k < vNodesInCut->nSize; k++) {
                int *pNum = (int *)Vec_PtrEntry(vNodesInCut, k);
                isrepeated = isrepeated & Vec_Ismemeber(vRow, *pNum);  // Corrected function name
                if (!isrepeated) { break; }
            }
        }
        if (isrepeated) { return 1; }  // Return immediately if found repeated
    }
    return 0;  // If no match found, return false
}

void vKLAdd(If_Man_t *p, If_Obj_t *pObj, Vec_Ptr_t *NodesInCut) {
    // temp container for NodesInCut pointers
    Vec_Ptr_t *NodesInCutNew = Vec_PtrAlloc(10);
    for (int k = 0; k < Vec_PtrSize(NodesInCut); k++) {
        int *pNum = (int *)Vec_PtrEntry(NodesInCut, k);
        If_Obj_t *curr_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, *pNum);
        Vec_PtrPush(NodesInCutNew, &curr_node_obj->Id);
    }
    Vec_PtrPush(pObj->vKLCut, NodesInCutNew);
}

int KLCompare(float AreaTemp, float DelayTemp, float EdgeTemp, float LeavesTemp, float PowerTemp, float MergeTemp, If_Obj_t *pObj) {
    if (pObj->KLArea < AreaTemp) {
        return 1;
    }
    // else if (pObj->KLLeaves < LeavesTemp) {
    //     return 1;
    // } else if (pObj->KLEdge < EdgeTemp) {
    //     return 1;
    // } else if (pObj->KLMerge < MergeTemp) {
    //     return 1;
    // }
    // } else if (pObj->KLLeaves < LeavesTemp) {
    //     return 1;
    // } else if (pObj->KLEdge < EdgeTemp) {
    //     return 1;
    // } else if (pObj->KLMerge < MergeTemp) {
    //     return 1;
    // } else if (pObj->KLDelay < DelayTemp) {
    //     return 1;
    // } else if (pObj->KLPower < PowerTemp) {
    //     return 1;
    // } else {
    //     return 0;
    // }
    return 0;
}

static Vec_Int_t * If_ManSaveRefs( If_Man_t * p )
{
    If_Obj_t * pObj;
    Vec_Int_t * vRefs = Vec_IntAlloc( Vec_PtrSize(p->vObjs) );
    int i;
    If_ManForEachObj( p, pObj, i )
        Vec_IntPush( vRefs, pObj->nRefs );
    return vRefs;
}

static void If_ManRestoreRefs( If_Man_t * p, Vec_Int_t * vRefs )
{
    If_Obj_t * pObj;
    int i;
    If_ManForEachObj( p, pObj, i )
        pObj->nRefs = Vec_IntEntry( vRefs, i );
}

// float If_CutAreaDeref( If_Man_t * p, If_Cut_t * pCut )
// {
//     If_Obj_t * pLeaf;
//     float Area;
//     int i;
//     Area = If_CutLutArea(p, pCut);
//     If_CutForEachLeaf( p, pCut, pLeaf, i )
//     {
//         assert( pLeaf->nRefs > 0 );
//         if ( --pLeaf->nRefs > 0 || !If_ObjIsAnd(pLeaf) )
//             continue;
//         Area += If_CutAreaDeref( p, If_ObjCutBest(pLeaf) );
//     }
//     return Area;
// }

float If_CutAreaRec( If_Man_t * p, Vec_Ptr_t * faninlist )
{
    If_Obj_t * pLeaf;
    float Area;
    int i;
    Area = 1;
    for (i = 0; i < faninlist->nSize; i++) {
        int curr_node = *(int *)faninlist->pArray[i];
        pLeaf = (If_Obj_t *)Vec_PtrEntry(p->vObjs, curr_node);
        if (--pLeaf->nRefs > 0 || !If_ObjIsAnd(pLeaf) || pLeaf->fSpec == 1 ) {
            continue;
        }
        // check if current node is a vice-output of a KL-Cut
        // the KL-Cut must be visited by it's main fanout
        if (pLeaf->vCutsWithNode != NULL && pLeaf->vCutsWithNode->nSize > 1) {
            int occupied = 0;
            for (int j = 0; j < pLeaf->vCutsWithNode->nSize; j++) {
                int coroot_node = *(int*)pLeaf->vCutsWithNode->pArray[j];
                If_Obj_t* pCoroot = (If_Obj_t*)Vec_PtrEntry(p->vObjs, coroot_node);
                if (pCoroot->fOccupy == 1) {
                    occupied = 1;
                    break;
                }
            }
            if (occupied) {
                continue;
            }
        }
        if (pLeaf->vBestKLCut !=NULL) {
            pLeaf->fSpec = 1;
            pLeaf->fOccupy = 1;
            // printf("Fanin list of node %d:  \n",pLeaf->Id);
            Vec_Ptr_t * faninlistNew = FaninCount(p, pLeaf->vBestKLCut);
            // for (int j = 0; j < faninlistNew->nSize; j++) {
            //     printf("%d ", *(int* )faninlistNew->pArray[j]);
            // }
            // printf("\n");
            Area = Area + If_CutAreaRec(p, faninlistNew);
            Vec_PtrFree( faninlistNew );
        }
    }
    return Area;
}

float If_CutEdgeRec(If_Man_t* p, Vec_Ptr_t* faninlist)
{
    If_Obj_t* pLeaf;
    float Edge;
    int i;
    Edge = faninlist->nSize;
    for (i = 0; i < faninlist->nSize; i++) {
        int curr_node = *(int*)faninlist->pArray[i];
        pLeaf = (If_Obj_t*)Vec_PtrEntry(p->vObjs, curr_node);
        if (--pLeaf->nRefs > 0 || !If_ObjIsAnd(pLeaf) || pLeaf->fSpec == 1) {
            continue;
        }
        // check if current node is a vice-output of a KL-Cut
        // the KL-Cut must be visited by it's main fanout
        if (pLeaf->vCutsWithNode != NULL && pLeaf->vCutsWithNode->nSize > 1) {
            int occupied = 0;
            for (int j = 0; j < pLeaf->vCutsWithNode->nSize; j++) {
                int coroot_node = *(int*)pLeaf->vCutsWithNode->pArray[j];
                If_Obj_t* pCoroot = (If_Obj_t*)Vec_PtrEntry(p->vObjs, coroot_node);
                if (pCoroot->fOccupy == 1) {
                    occupied = 1;
                    break;
                }
            }
            if (occupied) {
                continue;
            }
        }
        if (pLeaf->vBestKLCut != NULL) {
            pLeaf->fSpec = 1;
            pLeaf->fOccupy = 1;
            Vec_Ptr_t* faninlistNew = FaninCount(p, pLeaf->vBestKLCut);
            Edge = Edge + If_CutEdgeRec(p, faninlistNew);
            Vec_PtrFree( faninlistNew );
        }
    }
    return Edge;
}

void printfff(If_Obj_t *pObj, Vec_Ptr_t *NodesInCut) {
    printf("Nodes in Current Cut %d is : ", pObj->Id);
    for (int i = 0; i < NodesInCut->nSize; i++) {
        printf("%d ", *(int *)NodesInCut->pArray[i]);
    }
    printf("\n");
}

void vKLPara(If_Man_t *p, If_Obj_t *pObj, Vec_Ptr_t *NodesInCut) {

    Vec_Ptr_t *faninlist = FaninCount(p, NodesInCut);
    Vec_Int_t *vRefs = If_ManSaveRefs( p );
    // 1-update delay
    if (pObj->pFanin0->Type != 4) {
        pObj->KLDelay = 1.0;}
    else {
        int max_leaves_delay = 0;
        for (int i = 0; i < faninlist->nSize; i++) {
            int curr_node = *(int *)faninlist->pArray[i];
            If_Obj_t *curr_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, curr_node);
            if (max_leaves_delay <= curr_node_obj->Level) {
                max_leaves_delay = curr_node_obj->Level + 1;
            }
        }
        pObj->KLDelay = max_leaves_delay;
    }
    // 2-update area
    // Vec_Ptr_t *fanoutlist = FanoutCount(p, NodesInCut);
    // float sumofarea = 0.0; float sumofanouts = 0;
    // for (int i = 0; i < faninlist->nSize; i++) {
    //     int curr_node = *(int *)faninlist->pArray[i];
    //     If_Obj_t *curr_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, curr_node);
    //     if (curr_node_obj->Type == 4) {
    //         if (curr_node_obj->vKLCut != NULL) {
    //             sumofarea = sumofarea + curr_node_obj->vKLCut->nSize;
    //         } else {
    //             sumofarea = sumofarea + 1;
    //         }
    //     } else {
    //         sumofarea = sumofarea + 0;
    //     }
    // }
    // for (int i = 0; i < fanoutlist->nSize; i++) {
    //     int curr_node = *(int *)fanoutlist->pArray[i];
    //     If_Obj_t *curr_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, curr_node);
    //     sumofanouts = sumofanouts + curr_node_obj->vFanouts->nSize;
    // }
    // pObj->KLArea = (pObj->vKLCut->nSize + sumofarea) / sumofanouts;
    float area = If_CutAreaRec( p, faninlist );
    pObj->KLArea = area;
    //printfff(pObj, NodesInCut);
    // printf("The KL area of this Cut is %f: \n", area);

    // 3-update edge
    //float sumofedges = 0.0;
    //for (int i = 0; i < faninlist->nSize; i++) {
    //    int curr_node = *(int *)faninlist->pArray[i];
    //    If_Obj_t *curr_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, curr_node);
    //    sumofedges = sumofedges + curr_node_obj->KLEdge / curr_node_obj->EstRefs;
    //}
    //pObj->KLEdge = faninlist->nSize + sumofedges;
    If_ManRestoreRefs( p, vRefs );
    If_ManCleanMarkS( p );
    If_ManCleanMarkO( p );
    pObj->KLEdge = If_CutEdgeRec(p, faninlist);
    // 4-update nleaves
    pObj->KLLeaves = faninlist->nSize;
    // 5-update klMerge
    {
        Vec_Ptr_t *fanoutlist = FanoutCount(p, NodesInCut);
        pObj->KLMerge = fanoutlist->nSize ? (float)1 / fanoutlist->nSize : IF_INFINITY;
        Vec_PtrFree( fanoutlist );
    }
    If_ManRestoreRefs( p, vRefs );
    If_ManCleanMarkS( p );
    If_ManCleanMarkO( p );
    Vec_IntFree( vRefs );
    Vec_PtrFree( faninlist );
}

void  If_CoreRec(If_Man_t *p, If_Obj_t *pObj, int curr_node, int maxFanin,
                 int maxFanout, int root_level, Vec_Ptr_t *NodesInCut) {

    // current cut's fanin/fanout number
    Vec_Ptr_t *fanins = FaninCount(p, NodesInCut);
    Vec_Ptr_t *fanouts = FanoutCount(p, NodesInCut);
    int faninCount = fanins->nSize;
    int fanoutCount = fanouts->nSize;
    Vec_PtrFree( fanins );
    Vec_PtrFree( fanouts );

    // for the Cos, the fnaout number reduced to 1
    // auto *CoObj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, *(int *) pObj->vFanouts->pArray[0]);

    // only satisfy the IO limit will continue the recursive
    if (faninCount <= maxFanin && fanoutCount <= maxFanout && pObj->vKLCut->nSize < 64) {
        // add the current satisfied cut into vKLCut
        // make sure the NodesInCut is not repeated in vKLCut
        int ifcontinue = pObj->vKLCut->nSize==0 || !vKLCutRepeated(pObj, NodesInCut);
        // the first one always with original single fanout
        if (ifcontinue) {
            vKLAdd(p, pObj, NodesInCut);
            //printfff(pObj, NodesInCut);
            // Vec_Ptr_t * Fanouts = FanoutCount(p, NodesInCut);
            // printf("THe fanouts of this cut is: ");
            // for (int i = 0; i < Fanouts->nSize; i++) {
            //     printf("%d ", *(int *)Fanouts->pArray[i]);
            // }
            // printf("\n\n");
        }
        // update curr_node to it's fanin/fanout
        // if curr_node.level == root_level, use fanout + fanin
        // if curr_node.level < root_level, use fanin + fanout
        // if curr_node.level > root_level, use fanin
        If_Obj_t *curr_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, curr_node);
        if (curr_node_obj->fMark == 0) {
            // mark curr_node_obj as visited to avoid visited in the same recursive
            curr_node_obj->fMark = 1;
            // situation-1 the node level = root node
            // try fanout node first to find the parallel node
            // then the fanin nodes
            if (curr_node_obj->Level == root_level) {
                if (curr_node_obj->Type == 4) {
                    for (int i = 0; i < curr_node_obj->vFanouts->nSize; i++) {
                        Vec_Ptr_t *NodesInCutCopy = Vec_PtrAlloc(0);
                        Vec_PtrCopy(NodesInCutCopy, NodesInCut);
                        int fanout_node = *(int *)curr_node_obj->vFanouts->pArray[i];
                        if (!Vec_Ismemeber(NodesInCut,fanout_node)) {
                            Vec_PtrPushUnique(NodesInCutCopy, (int *)curr_node_obj->vFanouts->pArray[i]);
                            If_CoreRec(p,pObj,fanout_node,maxFanin,maxFanout,root_level,NodesInCutCopy);
                        }
                        Vec_PtrFree( NodesInCutCopy );
                    }
                    int curr_node0 = curr_node_obj->pFanin0->Id;
                    int curr_node1 = curr_node_obj->pFanin1->Id;
                    if (curr_node0 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node0) && curr_node_obj->pFanin0->Level <= root_level) {
                            Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin0->Id);
                            If_CoreRec(p,pObj,curr_node0,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                    if (curr_node1 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node1 ) && curr_node_obj->pFanin1->Level <= root_level) {
                            Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin1->Id);
                            If_CoreRec(p,pObj,curr_node1,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                }
            // situation-2 the node level < root level
            // try the fanout nodes, make sure the node level should <= root level
            // then try fanin nodes
            } else if (curr_node_obj->Level < root_level) {
                if (curr_node_obj->Type == 4) {
                    for (int i = 0; i < curr_node_obj->vFanouts->nSize; i++) {
                        Vec_Ptr_t *NodesInCutCopy = Vec_PtrAlloc(0);
                        Vec_PtrCopy(NodesInCutCopy, NodesInCut);
                        int fanout_node = *(int *)curr_node_obj->vFanouts->pArray[i];
                        If_Obj_t *fanout_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, fanout_node);
                        if (!Vec_Ismemeber(NodesInCut,fanout_node) && fanout_node_obj->Level <= root_level) {
                            Vec_PtrPushUnique(NodesInCutCopy, (int *)curr_node_obj->vFanouts->pArray[i]);
                            If_CoreRec(p,pObj,fanout_node,maxFanin,maxFanout,root_level,NodesInCutCopy);
                        }
                        Vec_PtrFree( NodesInCutCopy );
                    }
                    int curr_node0 = curr_node_obj->pFanin0->Id;
                    int curr_node1 = curr_node_obj->pFanin1->Id;
                    if (curr_node0 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node0)) {
                            Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin0->Id);
                            If_CoreRec(p,pObj,curr_node0,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                    if (curr_node1 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node1)) {
                            Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin1->Id);
                            If_CoreRec(p,pObj,curr_node1,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                }
            }
            // situation-3 the node level > root level
            // only try the fanin nodes, make sure the node level should <= root level
            else {
                if (curr_node_obj->Type == 4) {
                    int curr_node0 = curr_node_obj->pFanin0->Id;
                    int curr_node1 = curr_node_obj->pFanin1->Id;
                    int curr_node0_level = curr_node_obj->pFanin0->Level;
                    int curr_node1_level = curr_node_obj->pFanin1->Level;
                    if (curr_node0 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node0)) {
                            if (curr_node0_level <= root_level)
                                Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin0->Id);
                            If_CoreRec(p,pObj,curr_node0,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                    if (curr_node1 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node1)) {
                            if (curr_node1_level <= root_level)
                                Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin1->Id);
                            If_CoreRec(p,pObj,curr_node1,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                }
            }
        }
    } else {
        if ( NodesInCut->nSize > 0 )
            NodesInCut->nSize--;
    }
    //printf("faninCount = %d\n", faninCount);
}

/************************************core function****************************************/
/************************************core function****************************************/
/* the expanding of KL Cut seems not work very efficiency, the multi-output based on original */
/*ABC results seems more reasonable, so here the nulti-output cut is generated by adding fanouts*/
/*of certai nodes in the cut*/
void If_TransandSort(If_Man_t* p) {
    If_Obj_t *pObj; int i;
    If_ManForEachNode(p, pObj, i) {
        // initialize the vKLCut
        pObj->vKLCut = Vec_PtrAlloc(0);
        if (pObj->vBestKLCut == NULL) {
            pObj->vBestKLCut = pObj->CutBest.vNodesInCut;
            pObj->vBestKLFanins = Vec_PtrAlloc(0);
            pObj->vBestKLFanins = FaninCount(p, pObj->vBestKLCut);
            if (pObj->vBestKLFanins->nSize > pObj->CutBest.nLeaves) {
                pObj->vBestKLFanins = Vec_PtrAlloc(0);
                for (int j = 0; j < pObj->CutBest.nLeaves; j++) {
                    int pNum = pObj->CutBest.pLeaves[j];
                If_Obj_t *CurrNode = (If_Obj_t *) Vec_PtrEntry(p->vObjs, pNum);
                    Vec_PtrPushUnique(pObj->vBestKLFanins, &CurrNode->Id);
                }
            }
            pObj->vBestKLFanouts = Vec_PtrAlloc(0);
            // If_Cut_t *pCutTemp = &pObj->CutBest;
            // If_CutDAG(p, pCutTemp);
            pObj->vBestKLFanouts = FanoutCount(p, pObj->vBestKLCut);
        }
        /*******************************sort extend results***************************************/
        // sort the single LUT
        Vec_PtrSort(pObj->CutBest.vNodesInCut, NULL);
        // sort the best KL Cut
        Vec_PtrSort(pObj->vBestKLCut, NULL);
    }
}

int If_ExistUnExtNode(If_Man_t* p, int level, int mode) {
    // mode - 1: check if all visited
    // mode - 2: check if level all visited
    If_Obj_t *pObj; int i; int exist = 0;
    if (mode == 1) {
        If_ManForEachNode(p, pObj, i) {
            if (pObj->fVisit == 0 ) {
                exist = 1;
                break;
            }
        }
        return exist;
    }
    if (mode == 2) {
        If_ManForEachNode(p, pObj, i) {
            if (pObj->fVisit == 0 && (pObj->Level == level)) {
                exist = 1;
                break;
            }
        }
        return exist;
    }
    return 0;
}

void If_NearCutEnuRec(If_Man_t* p, int maxFanin, int maxFanout) {
    If_ManCleanMarkV(p); If_ManCleanMarkM(p); If_ManCleanMarkO(p);
    If_Obj_t *pObj; int i; int exist; int level = 1;
    while ( If_ExistUnExtNode(p, level, 1) ) {
        If_ManForEachNode(p, pObj, i) {
            exist = If_ExistUnExtNode(p, level, 2);
            if ( !exist ) { level++; }
            if (pObj->Level == level) {
                pObj->fVisit = 1;
            } else {
                continue;
            }
            // initialize the vKLCut
            pObj->vKLCut = Vec_PtrAlloc(0);
            //printf("\nProcessing node %d:\n",pObj->Id);
            // loop each node in the cut

            // int numlimit = round(1.0*(pObj->CutBest.vNodesInCut->nSize));
            for (int j = 0; j < 1; j++) {
            // for (int j = 0; j < pObj->CutBest.vNodesInCut->nSize; j++) {
                Vec_Ptr_t *NodesInCut = Vec_PtrAlloc(0);
                Vec_PtrCopy(NodesInCut, pObj->CutBest.vNodesInCut);
                //Vec_PtrPush(NodesInCut, &pObj->Id);
                int curr_node = *(int *)pObj->CutBest.vNodesInCut->pArray[j];
                int root_level = pObj->Level;
                If_ManCleanMarkM(p);
                If_CoreRec(p, pObj, curr_node, maxFanin, maxFanout, root_level, NodesInCut);
            }
            // select best KL cut
            // initialize KL cut parameters
            pObj->KLArea = IF_INFINITY;
            pObj->KLDelay = IF_INFINITY;
            pObj->KLLeaves = IF_INFINITY;
            pObj->KLEdge = IF_INFINITY;
            pObj->KLPower = IF_INFINITY;
            pObj->KLMerge= IF_INFINITY;
            // in a while loop until find the best cut
            int found = 0; Vec_Ptr_t *NodesInCutTemp = NULL;
            while (!found) {
                for (int j = 0; j < pObj->vKLCut->nSize; j++) {
                    Vec_Ptr_t *NodesInCut = (Vec_Ptr_t *)Vec_PtrEntry(pObj->vKLCut, j);
                    float AreaTemp = pObj->KLArea;
                    float DelayTemp = pObj->KLDelay;
                    float LeavesTemp = pObj->KLLeaves;
                    float EdgeTemp = pObj->KLEdge;
                    float PowerTemp = pObj->KLPower;
                    float MergeTemp = pObj->KLMerge;
                    If_ManCleanMarkS(p);
                    vKLPara(p, pObj, NodesInCut);
                    int ifbetter = KLCompare(AreaTemp, DelayTemp, EdgeTemp, LeavesTemp, PowerTemp, MergeTemp, pObj);
                    // if is better, update the best KL Cut
                    if (ifbetter == 1) {
                        pObj->vBestKLCut = Vec_PtrAlloc(0);
                        for (int k = 0; k < Vec_PtrSize(NodesInCut); k++) {
                            int pNum = *(int *)Vec_PtrEntry(NodesInCut, k);
                            If_Obj_t *CurrNode = (If_Obj_t *) Vec_PtrEntry(p->vObjs, pNum);
                            Vec_PtrPush(pObj->vBestKLCut, &CurrNode->Id);
                        }
                        pObj->vBestKLFanins = Vec_PtrAlloc(0);
                        pObj->vBestKLFanins = FaninCount(p, pObj->vBestKLCut);
                        pObj->vBestKLFanouts = Vec_PtrAlloc(0);
                        pObj->vBestKLFanouts = FanoutCount(p, pObj->vBestKLCut);
                    } else {
                        pObj->KLArea = AreaTemp;
                        pObj->KLDelay = DelayTemp;
                        pObj->KLLeaves = LeavesTemp;
                        pObj->KLEdge = EdgeTemp;
                        pObj->KLPower = PowerTemp;
                        pObj->KLMerge = MergeTemp;
                    }
                    NodesInCutTemp = NodesInCut;
                }

                if (pObj->vBestKLCut == NULL) {
                    pObj->vBestKLCut = pObj->CutBest.vNodesInCut;
                    pObj->vBestKLFanins = Vec_PtrAlloc(0);
                    pObj->vBestKLFanins = FaninCount(p, pObj->vBestKLCut);
                    pObj->vBestKLFanouts = Vec_PtrAlloc(0);
                    pObj->vBestKLFanouts = FanoutCount(p, pObj->vBestKLCut);
                }

                // deep copy to realize packing for percy mapping function
                If_Obj_t *tempObj = deepCopyIfObj(pObj);
                Vec_PtrCopy( tempObj->CutBest.vNodesInCut, pObj->vBestKLCut);
                If_Cut_t *pCutTemp = &tempObj->CutBest;
                If_CutDAG(p, pCutTemp);
                // int status = percy_map(pCutTemp);
                int status = 1;
                if (status != 1) {
                    printf("Percy Verification failed!\n");
                    //remove the current extended solution
                    if (pObj->vKLCut->nSize !=1) {
                        printf("Nodes in failed KL Cut: ");
                        for (int k = 0; k < NodesInCutTemp->nSize; k++) {
                            int *pNum = (int *)Vec_PtrEntry(NodesInCutTemp, k);
                            printf("%d ", *pNum);
                        }
                        printf("\n");
                        Vec_PtrRemove( pObj->vKLCut, NodesInCutTemp );
                    } else {
                        // if the only one left, delete some nodes
                        Vec_Ptr_t *NodesInCut = (Vec_Ptr_t *)Vec_PtrEntry(pObj->vKLCut, 0);
                        NodesInCut->nSize = NodesInCut->nSize - 1;
                    }
                } else {
                    printf("Percy Verification successful!\n");
                    found = true;
                    // generate the info vCutsWithNode
                    // document that the coRoot nodes exists in current pObj
                    for (int k = 0; k < pObj->vBestKLFanouts->nSize; k++) {
                        If_Obj_t *CoObj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, *(int *) pObj->vBestKLFanouts->pArray[k]);
                        if (CoObj->vCutsWithNode == NULL) {CoObj->vCutsWithNode = Vec_PtrAlloc(0);}
                        Vec_PtrPushUnique(CoObj->vCutsWithNode, &pObj->Id);
                    }
                }
            }
            Vec_PtrClear(pObj->vKLCut);
            /*******************************print extend results***************************************/
            // print the single LUT
            Vec_PtrSort(pObj->CutBest.vNodesInCut, NULL);
            // printf("Original LUT of Node %d: ", pObj->Id);
            // for (int j = 0; j < pObj->CutBest.vNodesInCut->nSize; j++) {
            //     int pNum = *(int *)Vec_PtrEntry(pObj->CutBest.vNodesInCut, j);
            //     printf("%d ", pNum);
            // }
            // // print the best KL Cut fanouts
            // printf("\nBest KL Cut fanouts %d: ", pObj->Id);
            // for (int j = 0; j < pObj->vBestKLFanouts->nSize; j++) {
            //     int pNum = *(int *)Vec_PtrEntry(pObj->vBestKLFanouts, j);
            //     printf("%d ", pNum);
            // }
            // // print the best KL Cut
            Vec_PtrSort(pObj->vBestKLCut, NULL);
            // printf("\nBest KL Cut of Node %d: ", pObj->Id);
            // for (int j = 0; j < pObj->vBestKLCut->nSize; j++) {
            //     int pNum = *(int *)Vec_PtrEntry(pObj->vBestKLCut, j);
            //     printf("%d ", pNum);
            // }
            // int fanoutCount = FanoutCount(p, pObj->vBestKLCut)->nSize;
            // int faninCount = FaninCount(p, pObj->vBestKLCut)->nSize;
            // printf("with %d fanouts and %d fanins\n\n", fanoutCount, faninCount);
        }
    }
    If_ManCleanMarkM(p); If_ManCleanMarkV(p);
}


int If_ManCoverNum( If_Man_t *p, If_Obj_t *leafObj, Vec_Ptr_t *nleaves, int maxfanin, int maxfanout) {
    // first, find the most coverage fanouts solution for a node
    Vec_Ptr_t *faninlist = Vec_PtrAlloc(0);
    // Vec_Ptr_t *fanoutlist = Vec_PtrAlloc(0);
    if (leafObj->vBestKLCut != NULL) {
        Vec_PtrPushUnique(faninlist, &leafObj->Id);
        for (int i = 0; i < leafObj->vBestKLCut->nSize; i++) {
            int nodetemp = *(int *)leafObj->vBestKLCut->pArray[i];
            auto *currentNode = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
            if (currentNode->Type == 4) {
                if (Vec_Ismemeber(leafObj->vBestKLCut,currentNode->pFanin0->Id)
                    && currentNode->pFanin0->vFanouts->nSize > 1 && currentNode->pFanin0->Type == 4)
                    Vec_PtrPushUnique(faninlist, &currentNode->pFanin0->Id);
                if (Vec_Ismemeber(leafObj->vBestKLCut,currentNode->pFanin1->Id)
                    && currentNode->pFanin1->vFanouts->nSize > 1 && currentNode->pFanin1->Type == 4)
                    Vec_PtrPushUnique(faninlist, &currentNode->pFanin1->Id);
            }
        }
        // // filter nodes only appear once in the cut
        // for (int i = 0; i < faninlist->nSize; i++) {
        //     int nodetemp1 = *(int *)faninlist->pArray[i];
        //     auto *currentNode1 = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp1);
        //     int asfanins = 0;
        //     for (int j = 0; j < leafObj->vBestKLCut->nSize; j++) {
        //         int nodetemp2 = *(int *)leafObj->vBestKLCut->pArray[i];
        //         if (nodetemp2 == nodetemp1) { continue;}
        //         auto *currentNode2 = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp2);
        //         if (nodetemp1 == currentNode2->pFanin0->Id || nodetemp1 == currentNode2->pFanin1->Id)
        //             asfanins++;
        //     }
        //     if (asfanins == 1)
        //         Vec_PtrPush(fanoutlist, &currentNode1->Id);
        // }

    }
    // else {
    //     Vec_PtrPushUnique(faninlist, &leafObj->Id);
    // }

    int covernum = 0;
    for (int i = 0; i < nleaves->nSize; i++) {
        int *pNum1 = (int *)Vec_PtrEntry(nleaves, i );
        if (leafObj->Type == 4) {
            for (int j = 0; j < faninlist->nSize; j++) {
                int *pNum2 = (int *)Vec_PtrEntry(faninlist, j );
                if (*pNum1 == *pNum2) {
                    covernum++;
                }
            }
        } else {
            if (*pNum1 == leafObj->Id) {
                covernum++;
                // if (leafObj->vBestKLFanouts == NULL) {leafObj->vBestKLFanouts = Vec_PtrAlloc(0);}
                // Vec_PtrPushUnique(leafObj->vBestKLFanouts, &leafObj->Id);
            }
        }
    }
    Vec_PtrFree( faninlist );
    if ( covernum > maxfanout )
        covernum = maxfanout;
    return covernum;
}


int If_ManCoverNumKL( If_Obj_t *leafObj, Vec_Ptr_t *nleaves) {
    int covernum = 0;
    for (int i = 0; i < nleaves->nSize; i++) {
        int *pNum1 = (int *)Vec_PtrEntry(nleaves, i );
        if (leafObj->Type == 4) {
            for (int j = 0; j < leafObj->vBestKLFanouts->nSize; j++) {
                int *pNum2 = (int *)Vec_PtrEntry(leafObj->vBestKLFanouts, j );
                if (*pNum1 == *pNum2) {covernum++;}
            }
        } else {
            if (*pNum1 == leafObj->Id) {covernum++;}
        }
    }
    return covernum;
}

void If_ManDeepSet( If_Man_t *p, Vec_Ptr_t *Vec1, Vec_Ptr_t *Vec2) {
    // Vec_PtrClear(Vec1);
    for (int i = 0; i < Vec2->nSize; i++) {
        int nodetemp = *(int *)Vec2->pArray[i];
        auto *currentNode = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
        Vec_PtrPushUnique(Vec1, &currentNode->Id);
    }
}

static float If_MoMergedLutArea( If_Man_t * p, int nFanins );

static Vec_Ptr_t * If_MoCutLeaves( If_Man_t * p, If_Cut_t * pCut )
{
    Vec_Ptr_t * vLeaves = Vec_PtrAlloc( pCut->nLeaves );
    If_Obj_t * pLeaf;
    int i;
    If_CutForEachLeaf( p, pCut, pLeaf, i )
        Vec_PtrPushUniqueNumber( vLeaves, &pLeaf->Id );
    return vLeaves;
}

static int If_MoCutContainsLeaf( If_Cut_t * pCut, int ObjId )
{
    int i;
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        if ( pCut->pLeaves[i] == ObjId )
            return 1;
    return 0;
}

static int If_MoArchitectureCheck( If_Cut_t * pCut0, If_Obj_t * pRoot0,
                                   If_Cut_t * pCut1, If_Obj_t * pRoot1,
                                   int nUnionLeaves, int maxFanin )
{
    const int kDual = 6;
    if ( pCut0 == NULL || pCut1 == NULL ||
         pCut0->nLeaves > kDual || pCut1->nLeaves > kDual ||
         nUnionLeaves > maxFanin )
        return 0;
    if ( If_MoCutContainsLeaf(pCut0, pRoot1->Id) ||
         If_MoCutContainsLeaf(pCut1, pRoot0->Id) )
        return 0;
    return 1;
}

static float If_MoGroupAreaFlow( If_Man_t * p, Vec_Ptr_t * vFanins )
{
    If_Obj_t * pFanin;
    int * pFaninId;
    int i;
    float Area = If_MoMergedLutArea( p, Vec_PtrSize(vFanins) );
    Vec_PtrForEachEntry( int *, vFanins, pFaninId, i )
    {
        pFanin = If_ManObj( p, *pFaninId );
        if ( pFanin == NULL || If_ObjIsConst1(pFanin) )
            continue;
        if ( pFanin->nRefs == 0 )
            Area += If_ObjCutBest(pFanin)->Area;
        else
            Area += If_ObjCutBest(pFanin)->Area /
                    Abc_MaxFloat( pFanin->EstRefs, p->fEpsilon );
    }
    return Area;
}

/*
 * Re-evaluate a single-output cut using the dual-output partner selected in
 * the previous mapping round.  The two original cuts remain separate, so each
 * output keeps its own input-to-output timing paths.  Only the physical LUT
 * area and the union-input architecture constraint are shared.
 */
float If_MultiOutputCutArea( If_Man_t * p, If_Obj_t * pObj, If_Cut_t * pCut,
                            float Area, int Mode, int maxFanin )
{
    If_Obj_t * pPartner;
    If_Cut_t * pPartnerCut;
    Vec_Ptr_t * vLeaves0, * vLeaves1, * vUnion;
    int PartnerId, nShared;
    float GroupArea, SharedArea;

    if ( pObj == NULL || pCut == NULL || pObj->vBestKLFanouts == NULL ||
         Vec_PtrSize(pObj->vBestKLFanouts) != 2 )
        return Area;
    PartnerId = *(int *)Vec_PtrEntry( pObj->vBestKLFanouts, 0 );
    if ( PartnerId == pObj->Id )
        PartnerId = *(int *)Vec_PtrEntry( pObj->vBestKLFanouts, 1 );
    if ( PartnerId < 0 || PartnerId >= If_ManObjNum(p) )
        return Area;
    pPartner = If_ManObj( p, PartnerId );
    if ( pPartner == NULL || pPartner == pObj || !If_ObjIsAnd(pPartner) )
        return Area;
    pPartnerCut = If_ObjCutBest( pPartner );
    if ( pPartnerCut == NULL || pPartnerCut->nLeaves == 0 ||
         pPartnerCut->Delay > pPartner->Required + p->fEpsilon )
        return Area;

    vLeaves0 = If_MoCutLeaves( p, pCut );
    vLeaves1 = If_MoCutLeaves( p, pPartnerCut );
    vUnion = Vec_PtrCombine( vLeaves0, vLeaves1 );
    nShared = Vec_PtrSize(vLeaves0) + Vec_PtrSize(vLeaves1) - Vec_PtrSize(vUnion);
    if ( nShared == 0 ||
         !If_MoArchitectureCheck( pCut, pObj, pPartnerCut, pPartner,
                                  Vec_PtrSize(vUnion), maxFanin ) ||
         pCut->Delay > pObj->Required + p->fEpsilon )
    {
        Vec_PtrFree( vLeaves0 );
        Vec_PtrFree( vLeaves1 );
        Vec_PtrFree( vUnion );
        return Area;
    }

    GroupArea = If_MoGroupAreaFlow( p, vUnion );
    if ( Mode == 2 )
        SharedArea = Area - 0.5f *
            (If_CutLutArea(p, pCut) + If_CutLutArea(p, pPartnerCut) -
             If_MoMergedLutArea(p, Vec_PtrSize(vUnion)));
    else
        SharedArea = GroupArea / 2.0f;
    Vec_PtrFree( vLeaves0 );
    Vec_PtrFree( vLeaves1 );
    Vec_PtrFree( vUnion );
    return Abc_MinFloat( Area, Abc_MaxFloat(0.0f, SharedArea) );
}

int If_MultiOutputGroupMerge( If_Man_t *p, If_Obj_t *pObj0, If_Obj_t *pObj1,
                              int maxFanin, int maxFanout )
{
    Vec_Ptr_t *vNodes;
    Vec_Ptr_t *vFanins;
    Vec_Ptr_t *vFanouts;
    int *pRootId;
    int i;
    float GroupArea;

    if ( pObj0 == NULL || pObj1 == NULL || pObj0 == pObj1 ||
         pObj0->Type != IF_AND || pObj1->Type != IF_AND ||
         pObj0->vBestKLCut == NULL || pObj1->vBestKLCut == NULL ||
         pObj0->vBestKLFanins == NULL || pObj1->vBestKLFanins == NULL ||
         pObj0->vBestKLFanouts == NULL || pObj1->vBestKLFanouts == NULL ||
         Vec_PtrSize(pObj0->vBestKLFanouts) != 1 ||
         Vec_PtrSize(pObj1->vBestKLFanouts) != 1 )
        return 0;

    vNodes = Vec_PtrCombine( pObj0->vBestKLCut, pObj1->vBestKLCut );
    vFanins = Vec_PtrCombine( pObj0->vBestKLFanins, pObj1->vBestKLFanins );
    vFanouts = Vec_PtrCombine( pObj0->vBestKLFanouts, pObj1->vBestKLFanouts );
    if ( !If_MoArchitectureCheck( If_ObjCutBest(pObj0), pObj0,
                                  If_ObjCutBest(pObj1), pObj1,
                                  Vec_PtrSize(vFanins), maxFanin ) ||
         Vec_PtrSize(vFanins) > maxFanin ||
         Vec_PtrSize(vFanouts) != maxFanout ||
         Vec_Ismemeber(vFanins, pObj0->Id) ||
         Vec_Ismemeber(vFanins, pObj1->Id) )
    {
        Vec_PtrFree( vNodes );
        Vec_PtrFree( vFanins );
        Vec_PtrFree( vFanouts );
        return 0;
    }

    GroupArea = If_MoGroupAreaFlow( p, vFanins );
    Vec_PtrForEachEntry( int *, vFanouts, pRootId, i )
    {
        If_Obj_t *pRoot;
        if ( *pRootId < 0 || *pRootId >= If_ManObjNum(p) )
            continue;
        pRoot = If_ManObj( p, *pRootId );
        if ( pRoot == NULL || pRoot->vBestKLCut == NULL ||
             pRoot->vBestKLFanins == NULL || pRoot->vBestKLFanouts == NULL )
            continue;
        Vec_PtrClear( pRoot->vBestKLCut );
        Vec_PtrClear( pRoot->vBestKLFanins );
        Vec_PtrClear( pRoot->vBestKLFanouts );
        If_ManDeepSet( p, pRoot->vBestKLCut, vNodes );
        If_ManDeepSet( p, pRoot->vBestKLFanins, vFanins );
        If_ManDeepSet( p, pRoot->vBestKLFanouts, vFanouts );
        pRoot->KLLeaves = Vec_PtrSize(vFanins);
        pRoot->KLMerge = 1.0f / Vec_PtrSize(vFanouts);
        pRoot->KLArea = GroupArea / Vec_PtrSize(vFanouts);
        pRoot->CutBest.Area = pRoot->KLArea;
    }
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vFanins );
    Vec_PtrFree( vFanouts );
    return 1;
}

typedef struct If_MoPair_t_
{
    If_Obj_t * pObj0;
    If_Obj_t * pObj1;
    float Score;
} If_MoPair_t;

static int If_MoPairCompare( const void * p0, const void * p1 )
{
    If_MoPair_t * pPair0 = *(If_MoPair_t **)p0;
    If_MoPair_t * pPair1 = *(If_MoPair_t **)p1;
    if ( pPair0->Score != pPair1->Score )
        return pPair0->Score > pPair1->Score ? -1 : 1;
    if ( pPair0->pObj0->Id != pPair1->pObj0->Id )
        return pPair0->pObj0->Id < pPair1->pObj0->Id ? -1 : 1;
    if ( pPair0->pObj1->Id != pPair1->pObj1->Id )
        return pPair0->pObj1->Id < pPair1->pObj1->Id ? -1 : 1;
    return 0;
}

static float If_MoMergedLutArea( If_Man_t * p, int nFanins )
{
    if ( p->pPars->pLutLib && nFanins <= p->pPars->nLutSize )
        return p->pPars->pLutLib->pLutAreas[nFanins];
    return 1.0f;
}

static float If_MoPairScore( If_Man_t * p, If_Obj_t * pObj0, If_Obj_t * pObj1,
                             int maxFanin, int maxFanout )
{
    Vec_Ptr_t * vNodes;
    Vec_Ptr_t * vFanins;
    Vec_Ptr_t * vFanouts;
    int nShared;
    float AreaSaving;
    float DelayPenalty;
    float Arrival0, Arrival1;
    float OriginalArrival;
    float Score = -IF_INFINITY;

    if ( pObj0 == NULL || pObj1 == NULL ||
         pObj0->vBestKLCut == NULL || pObj1->vBestKLCut == NULL ||
         pObj0->vBestKLFanins == NULL || pObj1->vBestKLFanins == NULL ||
         pObj0->vBestKLFanouts == NULL || pObj1->vBestKLFanouts == NULL ||
         Vec_PtrSize(pObj0->vBestKLFanouts) != 1 ||
         Vec_PtrSize(pObj1->vBestKLFanouts) != 1 )
        return -IF_INFINITY;

    vNodes = Vec_PtrCombine( pObj0->vBestKLCut, pObj1->vBestKLCut );
    vFanins = Vec_PtrCombine( pObj0->vBestKLFanins, pObj1->vBestKLFanins );
    vFanouts = Vec_PtrCombine( pObj0->vBestKLFanouts, pObj1->vBestKLFanouts );
    nShared = Vec_PtrSize(pObj0->vBestKLFanins) +
              Vec_PtrSize(pObj1->vBestKLFanins) - Vec_PtrSize(vFanins);
    if ( nShared > 0 &&
         If_MoArchitectureCheck( If_ObjCutBest(pObj0), pObj0,
                                 If_ObjCutBest(pObj1), pObj1,
                                 Vec_PtrSize(vFanins), maxFanin ) &&
         Vec_PtrSize(vFanouts) == maxFanout &&
         !Vec_Ismemeber(vFanins, pObj0->Id) &&
         !Vec_Ismemeber(vFanins, pObj1->Id) )
    {
        AreaSaving = If_CutLutArea( p, If_ObjCutBest(pObj0) ) +
                     If_CutLutArea( p, If_ObjCutBest(pObj1) ) -
                     If_MoMergedLutArea( p, Vec_PtrSize(vFanins) );
        /*
         * Keep the two timing cones separate.  A leaf in one output cut does
         * not create a timing path to the other output merely because both
         * functions occupy the same physical LUT.
         */
        Arrival0 = If_CutDelay( p, pObj0, If_ObjCutBest(pObj0) );
        Arrival1 = If_CutDelay( p, pObj1, If_ObjCutBest(pObj1) );
        OriginalArrival = Abc_MaxFloat( If_ObjArrTime(pObj0), If_ObjArrTime(pObj1) );
        DelayPenalty = Abc_MaxFloat( 0.0f,
            Abc_MaxFloat(Arrival0, Arrival1) - OriginalArrival );
        if ( Arrival0 > pObj0->Required + p->fEpsilon ||
             Arrival1 > pObj1->Required + p->fEpsilon )
            Score = -IF_INFINITY;
        else if ( AreaSaving > p->fEpsilon )
            Score = 1000.0f * AreaSaving
                  +          nShared
                  - 0.1f   * DelayPenalty
                  - 0.001f * Vec_PtrSize(vFanins)
                  - 0.0001f * Vec_PtrSize(vNodes);
        if ( Score != Score )
            Score = -IF_INFINITY;
    }
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vFanins );
    Vec_PtrFree( vFanouts );
    return Score;
}

static void If_ManMatchMultiOutput( If_Man_t * p, Vec_Ptr_t * vSelected,
                                    int maxFanin, int maxFanout )
{
    const int nBucketScanMax = 64;
    const int nCandidatesMax = 128;
    const int nPairsPerObjMax = 8;
    Vec_Ptr_t * vBuckets = Vec_PtrStart( If_ManObjNum(p) );
    Vec_Ptr_t * vAllPairs = Vec_PtrAlloc( 16 );
    Vec_Ptr_t * vPairs = Vec_PtrAlloc( 16 );
    Vec_Ptr_t * vBest = Vec_PtrStart( If_ManObjNum(p) );
    Vec_Int_t * vPairCounts = Vec_IntStart( If_ManObjNum(p) );
    If_MoPair_t * pPair;
    int * pObjId, * pFaninId, * pPartnerId;
    int i, k;

    /* Index selected single-output groups by shared fanins. */
    Vec_PtrForEachEntry( int *, vSelected, pObjId, i )
    {
        If_Obj_t * pObj;
        if ( *pObjId < 0 || *pObjId >= If_ManObjNum(p) )
            continue;
        pObj = If_ManObj( p, *pObjId );
        if ( pObj == NULL || pObj->vBestKLFanins == NULL ||
             pObj->vBestKLFanouts == NULL ||
             Vec_PtrSize(pObj->vBestKLFanouts) != 1 )
            continue;
        Vec_PtrForEachEntry( int *, pObj->vBestKLFanins, pFaninId, k )
        {
            Vec_Ptr_t * vBucket;
            if ( *pFaninId < 0 || *pFaninId >= If_ManObjNum(p) )
                continue;
            vBucket = (Vec_Ptr_t *)Vec_PtrEntry( vBuckets, *pFaninId );
            if ( vBucket == NULL )
            {
                vBucket = Vec_PtrAlloc( 4 );
                Vec_PtrWriteEntry( vBuckets, *pFaninId, vBucket );
            }
            Vec_PtrPush( vBucket, &pObj->Id );
        }
    }

    /* Generate unique candidate pairs from shared-fanin buckets. */
    Vec_PtrForEachEntry( int *, vSelected, pObjId, i )
    {
        Vec_Ptr_t * vCandidates;
        Vec_Ptr_t * vObjPairs;
        If_Obj_t * pObj0;
        if ( *pObjId < 0 || *pObjId >= If_ManObjNum(p) )
            continue;
        pObj0 = If_ManObj( p, *pObjId );
        if ( pObj0 == NULL || pObj0->vBestKLFanins == NULL ||
             pObj0->vBestKLFanouts == NULL ||
             Vec_PtrSize(pObj0->vBestKLFanouts) != 1 )
            continue;
        vCandidates = Vec_PtrAlloc( 16 );
        vObjPairs = Vec_PtrAlloc( 16 );
        Vec_PtrForEachEntry( int *, pObj0->vBestKLFanins, pFaninId, k )
        {
            Vec_Ptr_t * vBucket;
            int b, nScan, iStart;
            if ( *pFaninId < 0 || *pFaninId >= If_ManObjNum(p) )
                continue;
            vBucket = (Vec_Ptr_t *)Vec_PtrEntry( vBuckets, *pFaninId );
            if ( vBucket == NULL )
                continue;
            nScan = Abc_MinInt( Vec_PtrSize(vBucket), nBucketScanMax );
            iStart = pObj0->Id % Vec_PtrSize(vBucket);
            for ( b = 0; b < nScan; b++ )
            {
                pPartnerId = (int *)Vec_PtrEntry( vBucket,
                    (iStart + b) % Vec_PtrSize(vBucket) );
                if ( *pPartnerId == pObj0->Id )
                    continue;
                Vec_PtrPushUnique( vCandidates, pPartnerId );
                if ( Vec_PtrSize(vCandidates) >= nCandidatesMax )
                    break;
            }
            if ( Vec_PtrSize(vCandidates) >= nCandidatesMax )
                break;
        }
        Vec_PtrForEachEntry( int *, vCandidates, pPartnerId, k )
        {
            If_Obj_t * pObj1 = If_ManObj( p, *pPartnerId );
            If_Obj_t * pObjLo = pObj0->Id < pObj1->Id ? pObj0 : pObj1;
            If_Obj_t * pObjHi = pObj0->Id < pObj1->Id ? pObj1 : pObj0;
            float Score = If_MoPairScore( p, pObjLo, pObjHi, maxFanin, maxFanout );
            if ( Score == -IF_INFINITY )
                continue;
            pPair = ABC_ALLOC( If_MoPair_t, 1 );
            pPair->pObj0 = pObjLo;
            pPair->pObj1 = pObjHi;
            pPair->Score = Score;
            Vec_PtrPush( vObjPairs, pPair );
        }
        Vec_PtrSort( vObjPairs, If_MoPairCompare );
        Vec_PtrForEachEntry( If_MoPair_t *, vObjPairs, pPair, k )
            if ( k < nPairsPerObjMax )
                Vec_PtrPush( vAllPairs, pPair );
            else
                ABC_FREE( pPair );
        Vec_PtrFree( vCandidates );
        Vec_PtrFree( vObjPairs );
    }

    /*
     * Retain a bounded high-quality candidate graph.  Processing pairs in
     * score order gives each object up to nPairsPerObjMax strong alternatives.
     */
    Vec_PtrSort( vAllPairs, If_MoPairCompare );
    Vec_PtrForEachEntry( If_MoPair_t *, vAllPairs, pPair, i )
    {
        int ObjId0 = pPair->pObj0->Id;
        int ObjId1 = pPair->pObj1->Id;
        If_MoPair_t * pPrev = i == 0 ? NULL :
            (If_MoPair_t *)Vec_PtrEntry( vAllPairs, i - 1 );
        if ( pPrev != NULL && pPrev->pObj0->Id == ObjId0 &&
             pPrev->pObj1->Id == ObjId1 )
            continue;
        if ( Vec_IntEntry(vPairCounts, ObjId0) >= nPairsPerObjMax ||
             Vec_IntEntry(vPairCounts, ObjId1) >= nPairsPerObjMax )
            continue;
        Vec_IntWriteEntry( vPairCounts, ObjId0, Vec_IntEntry(vPairCounts, ObjId0) + 1 );
        Vec_IntWriteEntry( vPairCounts, ObjId1, Vec_IntEntry(vPairCounts, ObjId1) + 1 );
        Vec_PtrPush( vPairs, pPair );
        if ( Vec_PtrEntry(vBest, ObjId0) == NULL )
            Vec_PtrWriteEntry( vBest, ObjId0, pPair );
        if ( Vec_PtrEntry(vBest, ObjId1) == NULL )
            Vec_PtrWriteEntry( vBest, ObjId1, pPair );
    }

    If_ManCleanMarkV( p );
    /* Commit pairs that are the first choice of both endpoints. */
    Vec_PtrForEachEntry( If_MoPair_t *, vPairs, pPair, i )
    {
        if ( Vec_PtrEntry(vBest, pPair->pObj0->Id) != pPair ||
             Vec_PtrEntry(vBest, pPair->pObj1->Id) != pPair )
            continue;
        if ( pPair->pObj0->fVisit || pPair->pObj1->fVisit )
            continue;
        if ( If_MultiOutputGroupMerge( p, pPair->pObj0, pPair->pObj1,
                                       maxFanin, maxFanout ) )
        {
            pPair->pObj0->fVisit = 1;
            pPair->pObj1->fVisit = 1;
        }
    }

    /* Fill remaining unmatched objects greedily in descending score order. */
    Vec_PtrForEachEntry( If_MoPair_t *, vPairs, pPair, i )
    {
        if ( pPair->pObj0->fVisit || pPair->pObj1->fVisit )
            continue;
        if ( If_MultiOutputGroupMerge( p, pPair->pObj0, pPair->pObj1,
                                       maxFanin, maxFanout ) )
        {
            pPair->pObj0->fVisit = 1;
            pPair->pObj1->fVisit = 1;
        }
    }

    Vec_PtrForEachEntry( If_MoPair_t *, vAllPairs, pPair, i )
        ABC_FREE( pPair );
    for ( i = 0; i < Vec_PtrSize(vBuckets); i++ )
        if ( Vec_PtrEntry(vBuckets, i) != NULL )
            Vec_PtrFree( (Vec_Ptr_t *)Vec_PtrEntry(vBuckets, i) );
    Vec_PtrFree( vBuckets );
    Vec_PtrFree( vAllPairs );
    Vec_PtrFree( vPairs );
    Vec_PtrFree( vBest );
    Vec_IntFree( vPairCounts );
}

static void If_ManMultiOutputResetNode( If_Man_t * p, If_Obj_t * pObj )
{
    If_Cut_t * pCut = If_ObjCutBest( pObj );
    If_Obj_t * pLeaf;
    int i;
    if ( pObj->vBestKLCut == NULL )
        pObj->vBestKLCut = Vec_PtrAlloc( 8 );
    if ( pObj->vBestKLFanins == NULL )
        pObj->vBestKLFanins = Vec_PtrAlloc( 8 );
    if ( pObj->vBestKLFanouts == NULL )
        pObj->vBestKLFanouts = Vec_PtrAlloc( 2 );
    Vec_PtrClear( pObj->vBestKLCut );
    Vec_PtrClear( pObj->vBestKLFanins );
    Vec_PtrClear( pObj->vBestKLFanouts );
    if ( pCut->vNodesInCut != NULL )
        If_ManDeepSet( p, pObj->vBestKLCut, pCut->vNodesInCut );
    else
        Vec_PtrPush( pObj->vBestKLCut, &pObj->Id );
    If_CutForEachLeaf( p, pCut, pLeaf, i )
        Vec_PtrPushUniqueNumber( pObj->vBestKLFanins, &pLeaf->Id );
    Vec_PtrPush( pObj->vBestKLFanouts, &pObj->Id );
    pObj->KLArea = pCut->Area;
    pObj->KLDelay = pCut->Delay;
    pObj->KLEdge = pCut->Edge;
    pObj->KLPower = pCut->Power;
    pObj->KLLeaves = pCut->nLeaves;
    pObj->KLMerge = 1.0f;
}

int If_ManMultiOutputRound( If_Man_t * p, int maxFanin, int maxFanout )
{
    Vec_Ptr_t * vSelected = Vec_PtrAlloc( 128 );
    If_Obj_t * pObj;
    int i, nGroups = 0;
    float AreaSaving = 0.0f;
    If_ManForEachNode( p, pObj, i )
    {
        If_ManMultiOutputResetNode( p, pObj );
        if ( pObj->nRefs > 0 )
            Vec_PtrPush( vSelected, &pObj->Id );
    }
    If_ManMatchMultiOutput( p, vSelected, maxFanin, maxFanout );
    If_ManForEachNode( p, pObj, i )
        if ( Vec_PtrSize(pObj->vBestKLFanouts) == maxFanout &&
             *(int *)Vec_PtrEntry(pObj->vBestKLFanouts, 0) == pObj->Id )
        {
            If_Obj_t * pRoot1 = If_ManObj( p,
                *(int *)Vec_PtrEntry(pObj->vBestKLFanouts, 1) );
            AreaSaving += If_CutLutArea( p, If_ObjCutBest(pObj) ) +
                          If_CutLutArea( p, If_ObjCutBest(pRoot1) ) -
                          If_MoMergedLutArea( p,
                              Vec_PtrSize(pObj->vBestKLFanins) );
            nGroups++;
        }
    p->AreaGlo = Abc_MaxFloat( 0.0f, p->AreaGlo - AreaSaving );
    Vec_PtrFree( vSelected );
    return nGroups;
}

void If_ManSelRec(If_Man_t *p, Vec_Ptr_t *nleaves, Vec_Ptr_t *solutions, int levelleaf,
    Vec_Ptr_t *coveredLeaves, int type, int maxfanin, int maxfanout) {
    const int nGreedyLayerMax = 2048;
    const int nRecursionMax = 2048;
    Vec_Ptr_t *selectedCuts = Vec_PtrAlloc(0);
    if ( levelleaf >= nRecursionMax )
    {
        Vec_PtrFree( selectedCuts );
        return;
    }
    if (nleaves->nSize > 0) {
        if ( Vec_PtrSize(nleaves) > nGreedyLayerMax )
        {
            int * pObjId;
            int i;
            Vec_PtrForEachEntry( int *, nleaves, pObjId, i )
            {
                If_Obj_t * pObj;
                if ( *pObjId < 0 || *pObjId >= If_ManObjNum(p) )
                    continue;
                pObj = If_ManObj( p, *pObjId );
                if ( pObj == NULL )
                    continue;
                Vec_PtrPushUnique( coveredLeaves, &pObj->Id );
                if ( pObj->Type == IF_AND )
                {
                    Vec_PtrPushUnique( solutions, &pObj->Id );
                    Vec_PtrPushUnique( selectedCuts, &pObj->Id );
                }
            }
            Vec_PtrClear( nleaves );
        }
        else while (nleaves->nSize > 0) {
            int covernumax = -1;
            int bestObj = *(int *)Vec_PtrEntry(nleaves, 0);
            for (int i = 0; i < nleaves->nSize; i++) {
                auto *leafObj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, *(int *) nleaves->pArray[i]);
                int covernum = 0;
                if (type == 2)
                    covernum = If_ManCoverNumKL(leafObj, nleaves);
                else if (type == 1)
                    covernum = If_ManCoverNum(p, leafObj, nleaves, maxfanin, maxfanout);
                if (covernumax < covernum) {
                    covernumax = covernum;
                    bestObj = *(int *) nleaves->pArray[i];
                }
            }
            auto *bestleafObj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, bestObj);
            Vec_PtrPushUnique(coveredLeaves, &bestleafObj->Id);
            if (bestleafObj->Type == 4) {
                Vec_PtrPushUnique(solutions, &bestleafObj->Id);
                Vec_PtrPushUnique(selectedCuts, &bestleafObj->Id);
            }
            if (bestleafObj->Type == 4) {
                // remove the leaves from current best KL cut fanouts
                for (int i = 0; i < bestleafObj->vBestKLFanouts->nSize; i++) {
                    for (int j = 0; j < nleaves->nSize; j++) {
                        int *pNum1 = (int *)Vec_PtrEntry(nleaves, j);
                        int *pNum2 = (int *) Vec_PtrEntry(bestleafObj->vBestKLFanouts, i);
                        if (*pNum1 == *pNum2) {
                            Vec_PtrRemove(nleaves, pNum1);
                        }
                    }
                }
            } else {
                for (int j = 0; j < nleaves->nSize; j++) {
                    int *pNum1 = (int *)Vec_PtrEntry(nleaves, j);
                    int *pNum2 = &bestleafObj->Id;
                    if (*pNum1 == *pNum2) {
                        Vec_PtrRemove(nleaves, pNum1);
                    }
                }
            }
            // for (int j = 0; j < nleaves->nSize; j++) {
            //     int pNum = *(int *)Vec_PtrEntry(nleaves, j);
            //     printf("%d ", pNum);
            // }
            // printf("Need \n");
        }
        if ( p->pPars->fVerbose )
            Abc_Print( 1, "Reduced leaves of level %d with node number %d\n",
                       levelleaf, selectedCuts->nSize );
        levelleaf++;

        /****************************multi-output cut matching**************************/
        If_ManMatchMultiOutput( p, selectedCuts, maxfanin, maxfanout );

        // update the nleaves based on current selections
        // if the netlists size is large apply rec each layer
        if (p->vObjs->nSize > 15000) {
            for (int i = 0; i < selectedCuts->nSize; i++) {
                auto *tempObj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, *(int *) selectedCuts->pArray[i]);
                if (tempObj->Type == 4) {
                    for (int j = 0; j < tempObj->vBestKLFanins->nSize; j++) {
                        int tempId = *(int *)tempObj->vBestKLFanins->pArray[j];
                        if (!Vec_Ismemeber(coveredLeaves, tempId))
                            Vec_PtrPushUnique(nleaves, &tempId);
                    }
                }
            }
        } else {
            // if the netlists size is small apply rec each nodee
            for (int i = 0; i < selectedCuts->nSize; i++) {
                auto *tempObj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, *(int *) selectedCuts->pArray[i]);
                if (tempObj->Type == 4) {
                    if (!Vec_Ismemeber(coveredLeaves, tempObj->pFanin0->Id))
                        Vec_PtrPushUnique(nleaves, &tempObj->pFanin0->Id);
                    if (!Vec_Ismemeber(coveredLeaves, tempObj->pFanin1->Id))
                        Vec_PtrPushUnique(nleaves, &tempObj->pFanin1->Id);
                }
            }
        }
        If_ManSelRec(p, nleaves, solutions, levelleaf, coveredLeaves, type, maxfanin, maxfanout);
    }
    Vec_PtrFree( selectedCuts );
}

void If_FaninCutComb(If_Man_t *p, int maxfanin, int maxfanout) {
    If_Obj_t * pObj; int i;
    If_ManForEachNode(p, pObj, i) {
        If_Obj_t * pObjF0 = pObj->pFanin0;
        If_Obj_t * pObjF1 = pObj->pFanin1;
        If_MultiOutputGroupMerge(p, pObjF0, pObjF1, maxfanin, maxfanout);
        Vec_PtrSort(pObj->vBestKLFanins, NULL);
        Vec_PtrSort(pObj->vBestKLCut, NULL);
        Vec_PtrSort(pObj->vBestKLFanouts, NULL);
    }
}

/*void If_FaninCutCombUnit(If_Man_t *p, If_Obj_t *pObj, int maxfanin, int maxfanout) {
    if (pObj->Type == 4) {
        If_Obj_t * pObjF0 = pObj->pFanin0;
        If_Obj_t * pObjF1 = pObj->pFanin1;
        if (pObjF0 != NULL && pObjF1 != NULL && pObjF0->Type == 4 && pObjF1->Type == 4) {
            if (pObjF0->vBestKLFanouts->nSize + pObjF1->vBestKLFanouts->nSize < maxfanout) {
                Vec_Ptr_t *leaves = Vec_PtrAlloc(0);
                Vec_Ptr_t *roots = Vec_PtrAlloc(0);
                Vec_Ptr_t *nodes = Vec_PtrAlloc(0);
                for (int j = 0; j < pObjF0->vBestKLFanins->nSize; j++) {
                    Vec_PtrPushUnique(leaves, pObjF0->vBestKLFanins->pArray[j]);
                }
                for (int j = 0; j < pObjF1->vBestKLFanins->nSize; j++) {
                    Vec_PtrPushUnique(leaves, pObjF1->vBestKLFanins->pArray[j]);
                }
                for (int j = 0; j < pObjF0->vBestKLFanouts->nSize; j++) {
                    Vec_PtrPushUnique(roots, pObjF0->vBestKLFanouts->pArray[j]);
                }
                for (int j = 0; j < pObjF1->vBestKLFanouts->nSize; j++) {
                    Vec_PtrPushUnique(roots, pObjF1->vBestKLFanouts->pArray[j]);
                }
                for (int j = 0; j < pObjF0->vBestKLCut->nSize; j++) {
                    Vec_PtrPushUnique(nodes, pObjF0->vBestKLCut->pArray[j]);
                }
                for (int j = 0; j < pObjF1->vBestKLCut->nSize; j++) {
                    Vec_PtrPushUnique(nodes, pObjF1->vBestKLCut->pArray[j]);
                }
                if (leaves->nSize <= maxfanin) {
                    // update the fanins and fanouts after merge
                    for (int j = 0; j < leaves->nSize; j++) {
                        auto *leafObj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, *(int *) leaves->pArray[j]);
                        if (pObjF0->Id != leafObj->Id)
                            Vec_PtrPushUnique(pObjF0->vBestKLFanins, &leafObj->Id);
                        if (pObjF1->Id != leafObj->Id)
                            Vec_PtrPushUnique(pObjF1->vBestKLFanins, &leafObj->Id);
                    }
                    for (int j = 0; j < roots->nSize; j++) {
                        auto *rootObj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, *(int *) roots->pArray[j]);
                        Vec_PtrPushUnique(pObjF0->vBestKLFanouts, &rootObj->Id);
                        Vec_PtrPushUnique(pObjF1->vBestKLFanouts, &rootObj->Id);
                    }
                    for (int j = 0; j < nodes->nSize; j++) {
                        auto *nodeObj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, *(int *) nodes->pArray[j]);
                        if (nodeObj->Type == 4)
                            Vec_PtrPushUnique(pObjF0->vBestKLCut, &nodeObj->Id);
                        Vec_PtrPushUnique(pObjF1->vBestKLCut, &nodeObj->Id);
                    }
                }
                Vec_PtrSort(pObjF0->vBestKLFanins, NULL);
                Vec_PtrSort(pObjF1->vBestKLFanins, NULL);
                Vec_PtrSort(pObjF0->vBestKLCut, NULL);
                Vec_PtrSort(pObjF1->vBestKLCut, NULL);
                Vec_PtrSort(pObjF0->vBestKLFanouts, NULL);
                Vec_PtrSort(pObjF1->vBestKLFanouts, NULL);
            }
        }
        Vec_PtrSort(pObj->vBestKLFanins, NULL);
        Vec_PtrSort(pObj->vBestKLCut, NULL);
        Vec_PtrSort(pObj->vBestKLFanouts, NULL);
    }
}

Vec_Ptr_t *If_TopNodeGen(If_Man_t *p, If_Obj_t * pObj, Vec_Ptr_t *topnodes) {

    if (pObj->Type != 4) {return topnodes;}

    If_Obj_t *pObjF0 = pObj->pFanin0;
    if (pObjF0->Type == 4 && !Vec_Ismemeber(pObj->vBestKLCut, pObjF0->Id))
        for (int i = 0; i < pObjF0->vBestKLFanins->nSize; i++) {
            int nodetemp = *(int *)pObjF0->vBestKLFanins->pArray[i];
            auto *currentNode = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
            if (currentNode->Type != 4) { continue; }
            for (int j = 0; j < currentNode->vFanouts->nSize; j++) {
                if (Vec_Ismemeber(pObjF0->vBestKLCut, *(int *)currentNode->vFanouts->pArray[j]))
                    Vec_PtrPushUnique(topnodes, (int *)currentNode->vFanouts->pArray[j]);
            }
        }

    If_Obj_t *pObjF1 = pObj->pFanin1;
    if (pObjF1->Type == 4 && !Vec_Ismemeber(pObj->vBestKLCut, pObjF1->Id))
        for (int i = 0; i < pObjF1->vBestKLFanins->nSize; i++) {
            int nodetemp = *(int *)pObjF1->vBestKLFanins->pArray[i];
            auto *currentNode = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
            if (currentNode->Type != 4) { continue; }
            for (int j = 0; j < currentNode->vFanouts->nSize; j++) {
                if (Vec_Ismemeber(pObjF1->vBestKLCut, *(int *)currentNode->vFanouts->pArray[j]))
                    Vec_PtrPushUnique(topnodes, (int *)currentNode->vFanouts->pArray[j]);
            }
        }

    return topnodes;
}

void If_ManSelRecTopNodes(If_Man_t *p, Vec_Ptr_t *ntopNodes, Vec_Ptr_t *solutions, int levelnode,
    Vec_Ptr_t *coveredNodes, int type, int maxfanin, int maxfanout) {

    printf("The nodes of top layer: ");
    for (int i = 0; i < ntopNodes->nSize; i++) {
        printf("%d ", *(int *)ntopNodes->pArray[i]);
    }
    printf("\n");
    Vec_Ptr_t *topNodesNew = Vec_PtrAlloc(0);
    for (int i = 0; i < ntopNodes->nSize; i++) {
        int nodetemp = *(int *)ntopNodes->pArray[i];
        auto *currentNode = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
        If_FaninCutCombUnit(p, currentNode, maxfanout, maxfanin);
        topNodesNew = If_TopNodeGen(p, currentNode, topNodesNew);
    }
    if (topNodesNew->nSize > 0)
        If_ManSelRecTopNodes(p, topNodesNew, solutions, levelnode, coveredNodes, type, maxfanout, maxfanin);
}*/


/*************************user define layer bease KL enumeration*************************/
int If_ManLayerCoverNumKL( Vec_Ptr_t *fanins, Vec_Ptr_t *nleaves) {
    int covernum = 0;
    for (int i = 0; i < nleaves->nSize; i++) {
        int *pNum1 = (int *)Vec_PtrEntry(nleaves, i );
        for (int j = 0; j < fanins->nSize; j++) {
            int *pNum2 = (int *)Vec_PtrEntry(fanins, j );
            if (*pNum1 == *pNum2) {covernum++;}
        }
    }
    return covernum;
}

void If_LayerBasedExpand( If_Man_t *p, Vec_Ptr_t *nleaves, int maxFanin, int maxFanout ) {

    Vec_PtrSort(nleaves, NULL);

    // printf("Nodes in current layer: ");
    // for (int i = 0; i < nleaves->nSize; i++) {
    //     printf("%d ", *(int *)nleaves->pArray[i]);
    // }
    // printf("\n");

    // generate the next layer's leaves based on original information
    Vec_Ptr_t *nleavesNew = Vec_PtrAlloc(0);
    Vec_Ptr_t *nleavesUpdate = Vec_PtrAlloc(0);
    for (int i = 0; i < nleaves->nSize; i++) {
        int nodetemp1 = *(int *)nleaves->pArray[i];
        auto *currentNode1 = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp1);
        for (int j = 0; j < currentNode1->vBestKLFanins->nSize; j++) {
            int nodetemp2 = *(int *)currentNode1->vBestKLFanins->pArray[j];
            auto *currentNode2 = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp2);
            if (currentNode2->Type == 4)
                Vec_PtrPushUnique(nleavesNew, &currentNode2->Id);
        }
    }

    // for each node in the nleaves, try expanding it
    // the rule of selecting cut: 1-cover more fanouts; 2-do not generate new fanin leaves
    Vec_Ptr_t *nleavesCopy = Vec_PtrAlloc(0);
    Vec_PtrCopy( nleavesCopy, nleaves );

    for (int i = 0; i < nleaves->nSize; i++) {
        int nodetemp = *(int *)nleaves->pArray[i];
        auto *pObj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
        if (pObj->fVisit == 1) {continue;}
        // initialize the vKLCut and enumerate
        pObj->vKLCut = Vec_PtrAlloc(0);
        for (int j = 0; j < 1; j++) {
            Vec_Ptr_t *NodesInCut = Vec_PtrAlloc(0);
            Vec_PtrCopy(NodesInCut, pObj->CutBest.vNodesInCut);
            int curr_node = *(int *)pObj->CutBest.vNodesInCut->pArray[j];
            int root_level = pObj->Level;
            If_ManCleanMarkM(p);
            If_CoreRec(p, pObj, curr_node, maxFanin, maxFanout, root_level, NodesInCut);
            Vec_PtrFree( NodesInCut );
            if  (pObj->vKLCut->nSize == 0)
                vKLAdd( p, pObj, pObj->CutBest.vNodesInCut );
        }
        // choose the best cut for the node
        int covernumax = 0; int klcutIndex = 0;
        for (int j = 0; j < pObj->vKLCut->nSize; j++) {
            Vec_Ptr_t * NodesInCut = (Vec_Ptr_t *)pObj->vKLCut->pArray[j];
            Vec_Ptr_t * Fanins = FaninCount(p, NodesInCut);
            Vec_Ptr_t * Fanouts = FanoutCount(p, NodesInCut);
            int ismem = 1;
            for (int k = 0; k < Fanins->nSize; k++) {
                int fanintemp = *(int *)Fanins->pArray[k];
                if (!Vec_Ismemeber(nleavesNew, fanintemp)) {
                    ismem = 0;
                    break;
                }
            }
            if ( ismem ) {
                int covernum = If_ManLayerCoverNumKL(Fanouts, nleavesCopy);
                if ( covernumax < covernum ) {
                    covernumax = covernum;
                    klcutIndex = j;
                }
            }
            Vec_PtrFree( Fanins );
            Vec_PtrFree( Fanouts );
        }
        Vec_Ptr_t *BestKLCut = (Vec_Ptr_t *)pObj->vKLCut->pArray[klcutIndex];
        Vec_Ptr_t * Fanouts = FanoutCount(p, BestKLCut);

        if (Fanouts->nSize > 1) {
            // print
            printf("\nThe nodes for current node %d is: ", pObj->Id);
            for (int j = 0; j < BestKLCut->nSize; j++) {
                printf("%d ", *(int *)BestKLCut->pArray[j]);
            }
            printf("\n");
            printf("The fanouts for current node %d is: ", pObj->Id);
            for (int j = 0; j < Fanouts->nSize; j++) {
                printf("%d ", *(int *)Fanouts->pArray[j]);
            }
            printf("\n");
        }

        // copy the best KL cut to all fanout nodes in this cut
        for (int j = 0; j < Fanouts->nSize; j++) {
            int nodetemp = *(int *)Fanouts->pArray[j];
            If_Obj_t *pObjFanout = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
            Vec_Ptr_t *BestFanins = FaninCount(p, BestKLCut);
            Vec_Ptr_t *BestFanouts = FanoutCount(p, BestKLCut);
            Vec_PtrCopy(pObjFanout->vBestKLCut, BestKLCut);
            Vec_PtrCopy(pObjFanout->vBestKLFanins, BestFanins);
            Vec_PtrCopy(pObjFanout->vBestKLFanouts, BestFanouts);
            Vec_PtrFree( BestFanins );
            Vec_PtrFree( BestFanouts );

            // mark the node has been set
            pObjFanout->fVisit = 1;
            // reduce the covered node in nleavesCopy
            for (int k = 0; k < nleavesCopy->nSize; k++) {
                if (*(int *)nleavesCopy->pArray[k] == nodetemp)
                    Vec_PtrRemove(nleavesCopy, (int *)nleavesCopy->pArray[k]);
            }
        }

        // update the leaves for next layer
        Vec_Ptr_t * Fanins = FaninCount(p, BestKLCut);
        for (int j = 0; j < Fanins->nSize; j++) {
            int nodetemp = *(int *)Fanins->pArray[j];
            If_Obj_t *pObjFanin = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
            if (pObjFanin->Type == 4)
                Vec_PtrPush(nleavesUpdate, &pObjFanin->Id);
        }

        // if (pObj->Type == 4) {
        //     if (pObj->pFanin0->Type == 4)
        //         Vec_PtrPushUnique(nleavesUpdate, &pObj->pFanin0->Id);
        //     if (pObj->pFanin1->Type == 4)
        //         Vec_PtrPushUnique(nleavesUpdate, &pObj->pFanin1->Id);
        // }

        Vec_PtrFree( Fanins );
        Vec_PtrFree( Fanouts );
        if ( nleavesCopy->nSize == 0 ) { break; }
    }

    if (nleavesUpdate->nSize > 0)
        If_LayerBasedExpand( p, nleavesUpdate, maxFanin, maxFanout );
    Vec_PtrFree( nleavesNew );
    Vec_PtrFree( nleavesUpdate );
    Vec_PtrFree( nleavesCopy );
}

void If_FanoutCutComb(If_Man_t *p, int maxfanin, int maxfanout) {
    Vec_Ptr_t *nleaves = Vec_PtrAlloc(0);
    If_Obj_t * pObj; int i;
    //collect the first layer leaves from fanouts
    If_ManForEachCo( p, pObj, i ) {
        if (pObj->pFanin0->vBestKLFanins != NULL) {
            for (int j = 0; j < pObj->pFanin0->vBestKLFanins->nSize; j++) {
                Vec_PtrPushUnique(nleaves, &pObj->pFanin0->Id);
            }
        }
    }
    /****************************same layer cut merging*****************************/
    If_ManCleanMarkV(p);
    for (int i = 0; i < nleaves->nSize; i++) {
        int nodetemp1 = *(int *)nleaves->pArray[i];
        If_Obj_t *currentNode1 = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp1);
        if (currentNode1->fVisit == 1) {continue;}
        for (int j = i+1; j < nleaves->nSize; j++) {
            int nodetemp2 = *(int *)nleaves->pArray[j];
            If_Obj_t *currentNode2 = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp2);
            if (currentNode2->fVisit == 1) {continue;}
            if (If_MultiOutputGroupMerge(p, currentNode1, currentNode2, maxfanin, maxfanout)) {
                currentNode1->fVisit = 1;
                currentNode2->fVisit = 1;
                break;
            }
        }
    }
    Vec_PtrFree( nleaves );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
