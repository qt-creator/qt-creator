#include <math.h>
#include <assert.h>
#include "sqlite3.h"


static void okapi_bm25(sqlite3_context *pCtx, int nVal, sqlite3_value **apVal) {
    assert(sizeof(int) == 4);

    const unsigned int *matchinfo = (const unsigned int *)sqlite3_value_blob(apVal[0]);
    int searchTextCol = sqlite3_value_int(apVal[1]);

    double K1 = ((nVal >= 3) ? sqlite3_value_double(apVal[2]) : 1.2);
    double B = ((nVal >= 4) ? sqlite3_value_double(apVal[3]) : 0.75);

    int P_OFFSET = 0;
    int C_OFFSET = 1;
    int X_OFFSET = 2;

    int termCount = matchinfo[P_OFFSET];
    int colCount = matchinfo[C_OFFSET];

    int N_OFFSET = X_OFFSET + 3*termCount*colCount;
    int A_OFFSET = N_OFFSET + 1;
    int L_OFFSET = (A_OFFSET + colCount);


    double totalDocs = matchinfo[N_OFFSET];
    double avgLength = matchinfo[A_OFFSET + searchTextCol];
    double docLength = matchinfo[L_OFFSET + searchTextCol];

    double sum = 0.0;

    for (int i = 0; i < termCount; i++) {
        int currentX = X_OFFSET + (3 * searchTextCol * (i + 1));
        double termFrequency = matchinfo[currentX];
        double docsWithTerm = matchinfo[currentX + 2];

        double idf = log(
            (totalDocs - docsWithTerm + 0.5) /
            (docsWithTerm + 0.5)
        );

        double rightSide = (
            (termFrequency * (K1 + 1)) /
            (termFrequency + (K1 * (1 - B + (B * (docLength / avgLength)))))
        );

        sum += (idf * rightSide);
    }

    sqlite3_result_double(pCtx, sum);
}

//
//  Created by Joshua Wilson on 27/05/14.
//  Copyright (c) 2014 Joshua Wilson. All rights reserved.
//  https://github.com/neozenith/sqlite-okapi-bm25
//
// This is an extension to the work of "Radford 'rads' Smith"
// found at: https://github.com/rads/sqlite-okapi-bm25
// which is covered by the MIT License
// http://opensource.org/licenses/MIT
// the following code shall also be covered by the same MIT License

static void okapi_bm25f(sqlite3_context *pCtx, int nVal, sqlite3_value **apVal) {
    assert(sizeof(int) == 4);

    const unsigned int *matchinfo = (const unsigned int *)sqlite3_value_blob(apVal[0]);


    //Setting the default values and ignoring argument based inputs so the extra
    //arguments can be the column weights instead.
    double K1 = 1.2;// ((nVal >= 3) ? sqlite3_value_double(apVal[2]) : 1.2);
    double B = 0.75;// ((nVal >= 4) ? sqlite3_value_double(apVal[3]) : 0.75);

    //For a good explanation fo the maths and how to choose these variables
    //http://stackoverflow.com/a/23161886/622276

    //NOTE: the rearranged order of parameters to match the order presented on
    //SQLite3 FTS3 documentation 'pcxnals' (http://www.sqlite.org/fts3.html#matchinfo)

    int P_OFFSET = 0;
    int C_OFFSET = 1;
    int X_OFFSET = 2;

    int termCount = matchinfo[P_OFFSET];
    int colCount = matchinfo[C_OFFSET];

    int N_OFFSET = X_OFFSET + 3*termCount*colCount;
    int A_OFFSET = N_OFFSET + 1;
    int L_OFFSET = (A_OFFSET + colCount);
//    int S_OFFSET = (L_OFFSET + colCount); //useful as a pseudo proximity weighting per field/column

    double totalDocs = matchinfo[N_OFFSET];

    double avgLength = 0.0;
    double docLength = 0.0;

    for (int col = 0; col < colCount; col++)
    {
        avgLength +=  matchinfo[A_OFFSET + col];
        docLength +=  matchinfo[L_OFFSET + col];
    }

    double epsilon = 1.0 / (totalDocs*avgLength);
    double sum = 0.0;

    for (int t = 0; t < termCount; t++) {
        for (int col = 0 ; col < colCount; col++)
        {
            int currentX = X_OFFSET + (3 * col * (t + 1));


            double termFrequency = matchinfo[currentX];
            double docsWithTerm = matchinfo[currentX + 2];

            double idf = log(
                             (totalDocs - docsWithTerm + 0.5) /
                             (docsWithTerm + 0.5)
                             );
            // "...terms appearing in more than half of the corpus will provide negative contributions to the final document score."
            //http://en.wikipedia.org/wiki/Okapi_BM25

            idf = (idf < 0) ? epsilon : idf; //common terms could have no effect (\epsilon=0.0) or a very small effect (\epsilon=1/NoOfTokens which asymptotes to 0.0)

            double rightSide = (
                                (termFrequency * (K1 + 1)) /
                                (termFrequency + (K1 * (1 - B + (B * (docLength / avgLength)))))
                                );

            rightSide += 1.0;
            //To comply with BM25+ that solves a lower bounding issue where large documents that match are unfairly scored as
            //having similar relevancy as short documents that do not contain as many terms
            //Yuanhua Lv and ChengXiang Zhai. 'Lower-bounding term frequency normalization.' In Proceedings of CIKM'2011, pages 7-16.
            //http://sifaka.cs.uiuc.edu/~ylv2/pub/cikm11-lowerbound.pdf

            double weight = ((nVal > col+1) ? sqlite3_value_double(apVal[col+1]) : 1.0);

//            double subsequence = matchinfo[S_OFFSET + col];

            sum += (idf * rightSide) * weight; // * subsequence; //useful as a pseudo proximty weighting
        }
    }

    sqlite3_result_double(pCtx, sum);
}

static void okapi_bm25f_kb(sqlite3_context *pCtx, int nVal, sqlite3_value **apVal) {
    assert(sizeof(int) == 4);

    const unsigned int *matchinfo = (const unsigned int *)sqlite3_value_blob(apVal[0]);


    //Setting the default values and ignoring argument based inputs so the extra
    //arguments can be the column weights instead.
    if (nVal < 2) sqlite3_result_error(pCtx, "wrong number of arguments to function okapi_bm25_kb(), expected k1 parameter", -1);
    if (nVal < 3) sqlite3_result_error(pCtx, "wrong number of arguments to function okapi_bm25_kb(), expected b parameter", -1);
    double K1 = sqlite3_value_double(apVal[1]);
    double B = sqlite3_value_double(apVal[2]);

    //For a good explanation fo the maths and how to choose these variables
    //http://stackoverflow.com/a/23161886/622276

    //NOTE: the rearranged order of parameters to match the order presented on
    //SQLite3 FTS3 documentation 'pcxnals' (http://www.sqlite.org/fts3.html#matchinfo)

    int P_OFFSET = 0;
    int C_OFFSET = 1;
    int X_OFFSET = 2;

    int termCount = matchinfo[P_OFFSET];
    int colCount = matchinfo[C_OFFSET];

    int N_OFFSET = X_OFFSET + 3*termCount*colCount;
    int A_OFFSET = N_OFFSET + 1;
    int L_OFFSET = (A_OFFSET + colCount);
    //    int S_OFFSET = (L_OFFSET + colCount); //useful as a pseudo proximity weighting per field/column

    double totalDocs = matchinfo[N_OFFSET];

    double avgLength = 0.0;
    double docLength = 0.0;

    for (int col = 0; col < colCount; col++)
    {
        avgLength +=  matchinfo[A_OFFSET + col];
        docLength +=  matchinfo[L_OFFSET + col];
    }

    double epsilon = 1.0 / (totalDocs*avgLength);
    double sum = 0.0;

    for (int t = 0; t < termCount; t++) {
        for (int col = 0 ; col < colCount; col++)
        {
            int currentX = X_OFFSET + (3 * col * (t + 1));


            double termFrequency = matchinfo[currentX];
            double docsWithTerm = matchinfo[currentX + 2];

            double idf = log(
                             (totalDocs - docsWithTerm + 0.5) /
                             (docsWithTerm + 0.5)
                             );
            // "...terms appearing in more than half of the corpus will provide negative contributions to the final document score."
            //http://en.wikipedia.org/wiki/Okapi_BM25

            idf = (idf < 0) ? epsilon : idf; //common terms could have no effect (\epsilon=0.0) or a very small effect (\epsilon=1/NoOfTokens which asymptotes to 0.0)

            double rightSide = (
                                (termFrequency * (K1 + 1)) /
                                (termFrequency + (K1 * (1 - B + (B * (docLength / avgLength)))))
                                );

            rightSide += 1.0;
            //To comply with BM25+ that solves a lower bounding issue where large documents that match are unfairly scored as
            //having similar relevancy as short documents that do not contain as many terms
            //Yuanhua Lv and ChengXiang Zhai. 'Lower-bounding term frequency normalization.' In Proceedings of CIKM'2011, pages 7-16.
            //http://sifaka.cs.uiuc.edu/~ylv2/pub/cikm11-lowerbound.pdf

            double weight = ((nVal > col+3) ? sqlite3_value_double(apVal[col+3]) : 1.0);

            //            double subsequence = matchinfo[S_OFFSET + col];

            sum += (idf * rightSide) * weight; // * subsequence; //useful as a pseudo proximty weighting
        }
    }

    sqlite3_result_double(pCtx, sum);
}
