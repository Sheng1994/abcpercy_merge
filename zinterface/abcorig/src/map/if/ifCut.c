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

#include "if.h"
#include "../../../../../zinterface/cnf_gen.cpp"

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
    // Deep copy vectors (assuming Vec_PtrDup works correctly for deep copying)
    if (pObj->vFanouts) {
        NewpObj->vFanouts = Vec_PtrDup(pObj->vFanouts);
    } else {
        NewpObj->vFanouts = NULL;
    }
    if (pObj->vCutsWithNode) {
        NewpObj->vCutsWithNode = Vec_PtrDup(pObj->vCutsWithNode);
    } else {
        NewpObj->vCutsWithNode = NULL;
    }
    if (pObj->CutBest.vNodesInCut) {
        NewpObj->CutBest.vNodesInCut = Vec_PtrDup(pObj->CutBest.vNodesInCut);
    } else {
        NewpObj->CutBest.vNodesInCut = NULL;
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
    for (int i = 0; i < Vec_PtrSize(pCut->netDAG); i++) {
        Vec_Ptr_t *vRow = (Vec_Ptr_t *)Vec_PtrEntry(pCut->netDAG, i);
        Vec_Ptr_t *vInv = (Vec_Ptr_t *)Vec_PtrEntry(pCut->DagOP, i);
        for (int j = 0; j < Vec_PtrSize(vRow); j++) {
            int *pNum = (int *)Vec_PtrEntry(vRow, j);
            int *pInv = (int *)Vec_PtrEntry(vInv, j);
            //printf("netDAG[%d][%d] = %d with %d inv\n", i, j, *pNum, *pInv);
        }
    }

    //printf("\n");
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
void If_CutFanoutVec(If_Man_t* p) {

    If_Obj_t * pObx;
    If_Obj_t * pOby;
    int x, y;

    // Ensure all objects have a fanouts vector initialized
    If_ManForEachObj(p, pOby, x) {
        if (pOby->vFanouts == NULL) {
            pOby->vFanouts = Vec_PtrAlloc(100); // Allocate once per object
        }
    }

    If_ManForEachObj(p, pObx, x) {
        if (pObx == NULL) continue;
        int currNodeId = pObx->Id;
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
int Vec_Ismemeber(Vec_Ptr_t *vNodesInCut,int numtemp) {
    for (int j = 0; j < vNodesInCut->nSize; j++) {
        int nodeindex = *(int *)vNodesInCut->pArray[j];
        if (nodeindex == numtemp) {
            return 1;
            break;
        }
    }
    return 0;
}

Vec_Ptr_t *FaninCount(If_Man_t *p, Vec_Ptr_t *vNodesInCut) {
    int faninCount = 0;
    Vec_Ptr_t *faninlist = Vec_PtrAlloc(0);
    Vec_Ptr_t *fanins = Vec_PtrAlloc(0);
    for (int i = 0; i < vNodesInCut->nSize; i++) {
        int nodetemp = *(int *)vNodesInCut->pArray[i];
        If_Obj_t *currentNode = (If_Obj_t *)Vec_PtrEntry(p->vObjs, nodetemp);
        Vec_PtrPushUnique(faninlist, &currentNode->pFanin0->Id);
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
    // Vec_PtrFree(faninlist);
    // return faninCount;
    return fanins;
}

Vec_Ptr_t *FanoutCount(If_Man_t *p, Vec_Ptr_t *vNodesInCut) {
    int fanoutCount = 0;
    Vec_Ptr_t *faninlist = Vec_PtrAlloc(10);
    Vec_Ptr_t *fanouts = Vec_PtrAlloc(10);
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
    // Vec_PtrFree(faninlist);
    // return fanoutCount;
    return fanouts;
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

int KLCompare(float AreaTemp, float DelayTemp, float EdgeTemp, float LeavesTemp, float PowerTemp, If_Obj_t *pObj) {
    if (pObj->KLDelay < AreaTemp) {
        return 1;
    } else if (pObj->KLArea < DelayTemp) {
        return 1;
    } else if (pObj->KLEdge < EdgeTemp) {
        return 1;
    } else if (pObj->KLLeaves < LeavesTemp) {
        return 1;
    } else if (pObj->KLPower < PowerTemp) {
        return 1;
    } else {
        return 0;
    }
}

void vKLPara(If_Man_t *p, If_Obj_t *pObj, Vec_Ptr_t *NodesInCut) {

    Vec_Ptr_t *faninlist = FaninCount(p, NodesInCut);
    // 1-update delay
    if (pObj->pFanin0->Type != 4) {
        pObj->KLDelay = 1.0;}
    else {
        int max_leaves_delay = 0;
        for (int i = 0; i < faninlist->nSize; i++) {
            int curr_node = *(int *)faninlist->pArray[i];
            If_Obj_t *curr_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, curr_node);
            max_leaves_delay = (max_leaves_delay < curr_node_obj->KLDelay) ? curr_node_obj->KLDelay + 1 :max_leaves_delay;
        }
        pObj->KLDelay = max_leaves_delay;
    }
    // 2-update area
    Vec_Ptr_t *fanoutlist = FanoutCount(p, NodesInCut);
    float sumofarea = 0.0; float sumofanouts = 0;
    for (int i = 0; i < faninlist->nSize; i++) {
        int curr_node = *(int *)faninlist->pArray[i];
        If_Obj_t *curr_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, curr_node);
        if (curr_node_obj->Type == 4) {
            if (curr_node_obj->vKLCut != NULL) {
                sumofarea = sumofarea + curr_node_obj->vKLCut->nSize;
            } else {
                sumofarea = sumofarea + 1;
            }
        } else {
            sumofarea = sumofarea + 0;
        }
    }
    for (int i = 0; i < fanoutlist->nSize; i++) {
        int curr_node = *(int *)fanoutlist->pArray[i];
        If_Obj_t *curr_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, curr_node);
        sumofanouts = sumofanouts + curr_node_obj->vFanouts->nSize;
    }
    pObj->KLArea = (pObj->vKLCut->nSize + sumofarea) / sumofanouts;
    // 3-update edge
    float sumofedges = 0.0;
    for (int i = 0; i < faninlist->nSize; i++) {
        int curr_node = *(int *)faninlist->pArray[i];
        If_Obj_t *curr_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, curr_node);
        sumofedges = sumofedges + curr_node_obj->KLEdge / curr_node_obj->EstRefs;
    }
    pObj->KLEdge = faninlist->nSize + sumofedges;
    // 4-update nleaves
    pObj->KLLeaves = faninlist->nSize;
}

void  If_CoreRec(If_Man_t *p, If_Obj_t *pObj, int curr_node, int maxFanin,
                 int maxFanout, int root_level, Vec_Ptr_t *NodesInCut) {
    // current cut's fanin/fanout number
    int faninCount = FaninCount(p, NodesInCut)->nSize;
    int fanoutCount = FanoutCount(p, NodesInCut)->nSize;

    // only satisfy the IO limit will continue the recursive
    if (faninCount <= maxFanin && fanoutCount <= maxFanout) {
        // add the current satisfied cut into vKLCut
        // make sure the NodesInCut is not repeated in vKLCut
        int ifcontinue = pObj->vKLCut->nSize==0 || !vKLCutRepeated(pObj, NodesInCut);
        // the first one always with original single fanout
        if (ifcontinue) {
            vKLAdd(p, pObj, NodesInCut);
         }
        // update curr_node to it's fanin/fanout
        // if curr_node.level == root_level, use fanout + fanin
        // if curr_node.level < root_level, use fanin + fanout
        // if curr_node.level > root_level, use fanin
        If_Obj_t *curr_node_obj = (If_Obj_t *)Vec_PtrEntry(p->vObjs, curr_node);
        if (curr_node_obj->fMark == 0) {
            if (curr_node_obj->Level == root_level) {
                if (curr_node_obj->Type == 4) {
                    for (int i = 0; i < curr_node_obj->vFanouts->nSize; i++) {
                        int curr_node = *(int *)curr_node_obj->vFanouts->pArray[i];
                        //Vec_PtrPushUnique(NodesInCut,curr_node_obj->vFanouts->pArray[i]);
                        If_CoreRec(p,pObj,curr_node,maxFanin,maxFanout,root_level,NodesInCut);
                    }
                    int curr_node0 = curr_node_obj->pFanin0->Id;
                    int curr_node1 = curr_node_obj->pFanin1->Id;
                    if (curr_node0 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node_obj->pFanin0->Id)) {
                            Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin0->Id);
                            If_CoreRec(p,pObj,curr_node0,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                    if (curr_node1 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node_obj->pFanin1->Id)) {
                            Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin1->Id);
                            If_CoreRec(p,pObj,curr_node1,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                }
            } else if (curr_node_obj->Level < root_level) {
                if (curr_node_obj->Type == 4) {
                    int curr_node0 = curr_node_obj->pFanin0->Id;
                    int curr_node1 = curr_node_obj->pFanin1->Id;
                    if (curr_node0 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node_obj->pFanin0->Id)) {
                            Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin0->Id);
                            If_CoreRec(p,pObj,curr_node0,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                    if (curr_node1 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node_obj->pFanin1->Id)) {
                            Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin1->Id);
                            If_CoreRec(p,pObj,curr_node1,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                    for (int i = 0; i < curr_node_obj->vFanouts->nSize; i++) {
                        int curr_node = *(int *)curr_node_obj->vFanouts->pArray[i];
                        //Vec_PtrPushUnique(NodesInCut,curr_node_obj->vFanouts->pArray[i]);
                        If_CoreRec(p,pObj,curr_node,maxFanin,maxFanout,root_level,NodesInCut);
                    }
                }
            }
            else {
                if (curr_node_obj->Type == 4) {
                    int curr_node0 = curr_node_obj->pFanin0->Id;
                    int curr_node1 = curr_node_obj->pFanin1->Id;
                    if (curr_node0 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node_obj->pFanin0->Id)) {
                            Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin0->Id);
                            If_CoreRec(p,pObj,curr_node0,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                    if (curr_node1 > p->vCis->nSize) {
                        if (!Vec_Ismemeber(NodesInCut,curr_node_obj->pFanin1->Id)) {
                            Vec_PtrPushUnique(NodesInCut,&curr_node_obj->pFanin1->Id);
                            If_CoreRec(p,pObj,curr_node1,maxFanin,maxFanout,root_level,NodesInCut);
                        }
                    }
                }
            }
        }
    } else {
        NodesInCut->nSize--;
    }
    //printf("faninCount = %d\n", faninCount);
}

void If_NearCutEnuRec(If_Man_t* p, int maxFanin, int maxFanout) {
    If_Obj_t *pObj; int i;
    If_ManForEachNode(p, pObj, i) {
        // initialize the vKLCut
        pObj->vKLCut = Vec_PtrAlloc(0);
        //printf("\nProcessing node %d:\n",pObj->Id);
        // loop each node in the cut
        for (int j = 0; j < pObj->CutBest.vNodesInCut->nSize; j++) {
            Vec_Ptr_t *NodesInCut = Vec_PtrAlloc(0);
            Vec_PtrCopy(NodesInCut, pObj->CutBest.vNodesInCut);
            const int curr_node = *(int *)pObj->CutBest.vNodesInCut->pArray[j];
            //if (curr_node == pObj->Id) {continue;}
            int root_level = pObj->Level;
            // initialize fMark for each object
            If_ManCleanMarkV( p );
            If_CoreRec(p, pObj, curr_node, maxFanin, maxFanout, root_level, NodesInCut);
        }
        // select best KL cut
        // initialize KL cut parameters
        pObj->KLArea = IF_INFINITY;
        pObj->KLDelay = IF_INFINITY;
        pObj->KLLeaves = IF_INFINITY;
        pObj->KLEdge = IF_INFINITY;
        pObj->KLPower = IF_INFINITY;
        // in a while loop until find the best cut
        bool found = false; Vec_Ptr_t *NodesInCutTemp = nullptr;
        while (!found) {
            for (int j = 0; j < pObj->vKLCut->nSize; j++) {
                Vec_Ptr_t *NodesInCut = (Vec_Ptr_t *)Vec_PtrEntry(pObj->vKLCut, j);
                float AreaTemp = pObj->KLArea;
                float DelayTemp = pObj->KLDelay;
                float LeavesTemp = pObj->KLLeaves;
                float EdgeTemp = pObj->KLEdge;
                float PowerTemp = pObj->KLPower;
                vKLPara(p, pObj, NodesInCut);
                int ifbetter = KLCompare(AreaTemp, DelayTemp, EdgeTemp, LeavesTemp, PowerTemp, pObj);
                // if is better, update the best KL Cut
                if (ifbetter == 1) {
                    pObj->vBestKLCut = Vec_PtrAlloc(0);
                    for (int k = 0; k < Vec_PtrSize(NodesInCut); k++) {
                        int pNum = *(int *)Vec_PtrEntry(NodesInCut, k);
                        If_Obj_t *CurrNode = (If_Obj_t *) Vec_PtrEntry(p->vObjs, pNum);
                        //Vec_PtrFree(pObj->vBestKLCut);
                        Vec_PtrPush(pObj->vBestKLCut, &CurrNode->Id);
                    }
                }
                NodesInCutTemp = NodesInCut;
            }
            // deep copy to realize packing for percy mapping function
            If_Obj_t *tempObj = deepCopyIfObj(pObj);
            Vec_PtrCopy( tempObj->CutBest.vNodesInCut, pObj->vBestKLCut);
            If_Cut_t *pCutTemp = &tempObj->CutBest;
            If_CutDAG(p, pCutTemp);
            int status = percy_map(pCutTemp);
            //int status = 1 ;
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
                // Vec_PtrCopy(pObj->vBestKLCut, pObj->CutBest.vNodesInCut);
                // found = true;
            } else {
                printf("Percy Verification successful!\n");
                found = true;
            }
        }
        // print the best KL Cut
        printf("Best KL Cut of Node %d: ", pObj->Id);
        for (int j = 0; j < pObj->vBestKLCut->nSize; j++) {
            int pNum = *(int *)Vec_PtrEntry(pObj->vBestKLCut, j);
            printf("%d ", pNum);
        }
        int fanoutCount = FanoutCount(p, pObj->vBestKLCut)->nSize;
        printf("with %d fanouts\n", fanoutCount);
        // remove extended cut failed percy
        // for (int j = 0; j < pObj->vKLCut->nSize; j++) {
        //     Vec_Ptr_t *NodesInCut = (Vec_Ptr_t *)Vec_PtrEntry(pObj->vKLCut, j);
        //     // percy verification
        //     // deep copy to realize packing for percy mapping function
        //     If_Obj_t *tempObj = deepCopyIfObj(pObj);
        //     Vec_PtrCopy( tempObj->CutBest.vNodesInCut, NodesInCut);
        //     // print the nodes in current extended cut
        //     printf("Extended Cut for node %d: ",pObj->Id);
        //     for (int k = 0; k < Vec_PtrSize(NodesInCut); k++) {
        //         int *pNum = (int *)Vec_PtrEntry(NodesInCut, k);
        //         printf("%d ", *pNum);
        //     }
        //     printf("\n");
        //     If_Cut_t *pCutTemp = &tempObj->CutBest;
        //     If_CutDAG(p, pCutTemp);
        //     int status = percy_map(pCutTemp);
        //     if (status != 1) {
        //         printf("Percy Verification failed!\n");
        //         // remove the current extended solution
        //         Vec_PtrRemove( pObj->vKLCut, NodesInCut );
        //     } else {
        //         printf("Percy Verification successful!\n");
        //     }
        // }
    }
    If_ManCleanMarkV( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END