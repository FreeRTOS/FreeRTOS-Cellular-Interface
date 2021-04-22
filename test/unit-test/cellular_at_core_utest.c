/*
 * cellular_library v0.0.1
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file cellular_at_core_utest.c
 * @brief Unit tests for functions in cellular_at_core.h.
 */

#include <string.h>
#include <stdbool.h>

#include "unity.h"

/* Include paths for public enums, structures, and macros. */
#include "cellular_platform.h"

#include "cellular_at_core.h"

/**
 * @brief Cellular sample prefix string input.
 */
#define CELLULAR_SAMPLE_PREFIX_STRING_INPUT               "+CPIN:READY"

/**
 * @brief Cellular sample prefix string wron input.
 */
#define CELLULAR_SAMPLE_PREFIX_STRING_WRONG_INPUT          "+CPINREADY"

/**
 * @brief Cellular sample prefix string large input.
 */
#define CELLULAR_SAMPLE_PREFIX_STRING_LARGE_INPUT          "+CPIN:Story for Littel Red Riding Hood: Once upon a time there was a dear little girl who was loved by every one who looked at her, but most of all by her grandmother, and there was nothing that she would not have given to the child. Once she gave her a little cap of red velvet, which suited her so well that she would never wear anything else. So she was always called Little Red Riding Hood."

/**
 * @brief Cellular sample prefix string output.
 */
#define CELLULAR_SAMPLE_PREFIX_STRING_OUTPUT               "READY"

/**
 * @brief Cellular sample string leading white space.
 */
#define CELLULAR_SAMPLE_STRING_LEADING_WHITE_SPACE         "    response"

/**
 * @brief Cellular sample string trailing white space.
 */
#define CELLULAR_SAMPLE_STRING_TRAILING_WHITE_SPACE        "response    "

/**
 * @brief Cellular sample string all white space.
 */
#define CELLULAR_SAMPLE_STRING_ALL_WHITE_SPACE             "    response    "

/**
 * @brief Cellular sample string double quote.
 */
#define CELLULAR_SAMPLE_STRING_DOUBLE_QUOTE                "\"response\""

/**
 * @brief Cellular sample string no white space.
 */
#define CELLULAR_SAMPLE_STRING_NO_WHITE_SPACE              "response"

/**
 * @brief Cellular sample string double quote.
 */
#define CELLULAR_SAMPLE_STRING_TWO_DOUBLE_QUOTE            "\"res\"p\"onse\""

/**
 * @brief Cellular sample token string input.
 */
#define CELLULAR_SAMPLE_TOKEN_STRING_INPUT                 "TEST_TOKEN:TOKEN1,TOKEN2,TOKEN3"

/**
 * @brief Cellular sample token string output.
 */
#define CELLULAR_SAMPLE_TOKEN_OUTPUT                       "TEST_TOKEN:TOKEN1"

/**
 * @brief Cellular sample string delimter first.
 */
#define CELLULAR_SAMPLE_STRING_DELIMTER_FIRST              ",TEST_TOKENTOKEN1TOKEN2"

/**
 * @brief Cellular sample string delimter first.
 */
#define CELLULAR_SAMPLE_WRONG_STRING_DELIMTER_FIRST        "TEST_TOKENTOKEN1TOKEN2"

/**
 * @brief Cellular sample hex string in capital.
 */
#define CELLULAR_SAMPLE_HEX_STRING                         "10AB"

/**
 * @brief Cellular sample hex string in lower case.
 */
#define CELLULAR_SAMPLE_HEX_LOWER_CASE_STRING              "10ab"

/**
 * @brief Cellular sample digit string.
 */
#define CELLULAR_SAMPLE_DIGIT_STRING                       "1234"

/**
 * @brief Cellular sample digit string with NULL.
 */
#define CELLULAR_SAMPLE_DIGIT_STRING_WITH_NULL             "1234\"\0\"123"

/**
 * @brief Cellular sample success code string.
 */
#define CELLULAR_SAMPLE_SUCCESS_CODE_STRING                "TEST1:SUCCESS FOR TRANSMISSION"

/**
 * @brief Cellular sample error code string.
 */
#define CELLULAR_SAMPLE_ERROR_CODE_STRING                  "TEST2:ERROR FOR TRANSMISSION"

/**
 * @brief Cellular sample strtol error case string.
 */
#define CELLULAR_SAMPLE_STRTOL_ERROR_CASE_STRING           "20210487 This is test"

/**
 * @brief Cellular sample strtol happy case string.
 */
#define CELLULAR_SAMPLE_STRTOL_HAPPY_CASE_STRING           "20210487"

/**
 * @brief Cellular sample strtol happy case number.
 */
#define CELLULAR_SAMPLE_STRTOL_HAPPY_CASE_NUMBER           ( 20210487 )

/**
 * @brief Cellular sample prefix string wron input.
 */
#define CELLULAR_SAMPLE_PREFIX_STRING_STAR_FIRST_INPUT     "*CPIN:READY"

/* ============================   UNITY FIXTURES ============================ */

/* Called before each test method. */
void setUp()
{
    
}

/* Called after each test method. */
void tearDown()
{
}

/* Called at the beginning of the whole suite. */
void suiteSetUp()
{
}

/* Called at the end of the whole suite. */
int suiteTearDown( int numFailures )
{
    return numFailures;
}

/* ========================================================================== */

/* static function declaration */


/* ========================================================================== */

/**
 * @brief Test that any NULL parameter causes Cellular_ATRemovePrefix to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemovePrefix_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemovePrefix( NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test happy path for Cellular_ATRemovePrefix to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATRemovePrefix_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

    /* Check that CELLULAR_BAD_PARAMETER is returned if the parameter ppString set to sample string is passed. */
    cellularStatus = Cellular_ATRemovePrefix( &pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );

    /* Test the result to check its functionality. */
    TEST_ASSERT_EQUAL_STRING( CELLULAR_SAMPLE_PREFIX_STRING_OUTPUT, pString );
}

/**
 * @brief Test the error path for Cellular_ATRemovePrefix to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemovePrefix_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = "";
    char * pWrongString = CELLULAR_SAMPLE_PREFIX_STRING_WRONG_INPUT;
    char * pLargeString = CELLULAR_SAMPLE_PREFIX_STRING_LARGE_INPUT;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemovePrefix( &pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemovePrefix( &pWrongString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemovePrefix( &pLargeString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATRemoveLeadingWhiteSpaces to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemoveLeadingWhiteSpaces_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemoveLeadingWhiteSpaces( NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test happy path for Cellular_ATRemoveLeadingWhiteSpaces to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATRemoveLeadingWhiteSpaces_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = CELLULAR_SAMPLE_STRING_LEADING_WHITE_SPACE;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemoveLeadingWhiteSpaces( &pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );

    /* Test the result to check its functionality. */
    TEST_ASSERT_EQUAL_STRING( CELLULAR_SAMPLE_STRING_NO_WHITE_SPACE, pString );
}

/**
 * @brief Test the error path for Cellular_ATRemoveLeadingWhiteSpaces to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemoveLeadingWhiteSpaces_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = "";

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemoveLeadingWhiteSpaces( &pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATRemoveTrailingWhiteSpaces to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemoveTrailingWhiteSpaces_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemoveTrailingWhiteSpaces( NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test happy path for Cellular_ATRemoveTrailingWhiteSpaces to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATRemoveTrailingWhiteSpaces_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char pString[] = CELLULAR_SAMPLE_STRING_TRAILING_WHITE_SPACE;
    /* Check that CELLULAR_BAD_PARAMETER is returned if the parameter ppString set to sample string is passed. */
    cellularStatus = Cellular_ATRemoveTrailingWhiteSpaces( pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );

    /* Test the result to check its functionality. */
    TEST_ASSERT_EQUAL_STRING( CELLULAR_SAMPLE_STRING_NO_WHITE_SPACE, pString );
}

/**
 * @brief Test the error path for Cellular_ATRemoveTrailingWhiteSpaces to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemoveTrailingWhiteSpaces_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char pString[] = "";

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemoveTrailingWhiteSpaces( pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATRemoveAllWhiteSpaces to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemoveAllWhiteSpaces_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemoveAllWhiteSpaces( NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test happy path for test_Cellular_ATRemoveAllWhiteSpaces_Happy_Path to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATRemoveAllWhiteSpaces_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char pString[] = CELLULAR_SAMPLE_STRING_ALL_WHITE_SPACE;

    /* Check that CELLULAR_BAD_PARAMETER is returned if the parameter ppString set to sample string is passed. */
    cellularStatus = Cellular_ATRemoveAllWhiteSpaces( pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );

    /* Test the result to check its functionality. */
    TEST_ASSERT_EQUAL_STRING( CELLULAR_SAMPLE_STRING_NO_WHITE_SPACE, pString );
}

/**
 * @brief Test the error path for Cellular_ATRemoveAllWhiteSpaces to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemoveAllWhiteSpaces_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char pString[] = "";

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemoveAllWhiteSpaces( pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATRemoveOutermostDoubleQuote to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemoveOutermostDoubleQuote_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemoveOutermostDoubleQuote( NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test happy path for Cellular_ATRemoveOutermostDoubleQuote to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATRemoveOutermostDoubleQuote_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = malloc( sizeof( char ) * ( strlen( CELLULAR_SAMPLE_STRING_DOUBLE_QUOTE ) + 1 ) );
    char * pStringSource = pString;
    strcpy( pString, CELLULAR_SAMPLE_STRING_DOUBLE_QUOTE );

    /* Check that CELLULAR_BAD_PARAMETER is returned if the parameter ppString set to sample string is passed. */
    cellularStatus = Cellular_ATRemoveOutermostDoubleQuote( &pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );

    /* Test the result to check its functionality. */
    TEST_ASSERT_EQUAL_STRING( CELLULAR_SAMPLE_STRING_NO_WHITE_SPACE, pString );
    free( pStringSource );
}

/**
 * @brief Test the error path for Cellular_ATRemoveOutermostDoubleQuote to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemoveOutermostDoubleQuote_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = "";

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemoveOutermostDoubleQuote( &pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATRemoveAllDoubleQuote to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemoveAllDoubleQuote_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemoveAllDoubleQuote( NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test happy path for Cellular_ATRemoveAllDoubleQuote to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATRemoveAllDoubleQuote_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = malloc( sizeof( char ) * ( strlen( CELLULAR_SAMPLE_STRING_TWO_DOUBLE_QUOTE ) + 1 ) );
    char * pStringSource = pString;
    strcpy( pString, CELLULAR_SAMPLE_STRING_TWO_DOUBLE_QUOTE );

    cellularStatus = Cellular_ATRemoveAllDoubleQuote( pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );

    /* Test the result to check its functionality. */
    TEST_ASSERT_EQUAL_STRING( CELLULAR_SAMPLE_STRING_NO_WHITE_SPACE, pString );
    free( pStringSource );
}

/**
 * @brief Test the error path for Cellular_ATRemoveAllDoubleQuote to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATRemoveAllDoubleQuote_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = "";

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATRemoveAllDoubleQuote( pString );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATGetNextTok to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATGetNextTok_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString;
    char * pTokOutput;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATGetNextTok( NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATGetNextTok( &pString, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATGetNextTok( NULL, &pTokOutput );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test happy path for Cellular_ATGetNextTok to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATGetNextTok_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = malloc( sizeof( char ) * ( strlen( CELLULAR_SAMPLE_TOKEN_STRING_INPUT ) + 1 ) );
    char * pStringSource = pString;
    char * pToken;
    strcpy( pString, CELLULAR_SAMPLE_TOKEN_STRING_INPUT );

    /* Check that CELLULAR_AT_SUCCESS is returned. */
    cellularStatus = Cellular_ATGetNextTok( & pString,
                                            & pToken );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
    /* Test the result to check its functionality. */
    TEST_ASSERT_EQUAL_STRING( CELLULAR_SAMPLE_TOKEN_OUTPUT, pToken );

    free( pStringSource );
}

/**
 * @brief Test the error path for Cellular_ATGetSpecificNextTok to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATGetSpecificNextTok_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString;
    const char * pDelimiter;
    char * pTokOutput;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATGetSpecificNextTok( NULL, pDelimiter, &pTokOutput );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATGetSpecificNextTok( & pString, NULL, &pTokOutput );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATGetSpecificNextTok( & pString, pDelimiter, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test happy path for Cellular_ATGetSpecificNextTok to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATGetSpecificNextTok_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    const char * pDelimiter = ",";
    char * pTokOutput;
    char * pString = malloc( sizeof( char ) * ( strlen( CELLULAR_SAMPLE_STRING_TWO_DOUBLE_QUOTE ) + 1 ) );
    char * pStringSource = pString;
    
    pString = malloc( sizeof( char ) * ( strlen( CELLULAR_SAMPLE_STRING_DELIMTER_FIRST ) + 1 ) );
    pStringSource = pString;
    strcpy( pString, CELLULAR_SAMPLE_STRING_DELIMTER_FIRST );
    /* Check that CELLULAR_AT_SUCCESS is returned. */
    cellularStatus = Cellular_ATGetSpecificNextTok( & pString, pDelimiter, & pTokOutput );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
    free( pStringSource );

    pString = malloc( sizeof( char ) * ( strlen( CELLULAR_SAMPLE_WRONG_STRING_DELIMTER_FIRST ) + 1 ) );
    pStringSource = pString;
    strcpy( pString, CELLULAR_SAMPLE_WRONG_STRING_DELIMTER_FIRST );
    /* Check that CELLULAR_AT_SUCCESS is returned. */
    cellularStatus = Cellular_ATGetSpecificNextTok( & pString, pDelimiter, & pTokOutput );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
    free( pStringSource );
}

/**
 * @brief Test the error path for Cellular_ATGetSpecificNextTok to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATGetSpecificNextTok_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = "";
    const char * pDelimiter = ",";
    char * pTokOutput;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATGetSpecificNextTok( & pString, pDelimiter, & pTokOutput );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATHexStrToHex to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATHexStrToHex_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString;
    uint8_t * const * const ppHexData;
    uint16_t hexDataLen;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATHexStrToHex( NULL, ppHexData, hexDataLen );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATHexStrToHex( pString, NULL, hexDataLen );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test happy path for Cellular_ATHexStrToHex to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATHexStrToHex_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char pString[] = CELLULAR_SAMPLE_HEX_STRING;
    char pLowerCaseString[] = CELLULAR_SAMPLE_HEX_LOWER_CASE_STRING;
    uint8_t * ppHexData[3];
    uint16_t hexDataLen = 3;

    /* Check that CELLULAR_AT_SUCCESS is returned. */
    cellularStatus = Cellular_ATHexStrToHex( pString, ppHexData, hexDataLen );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );    

    /* Check that CELLULAR_AT_SUCCESS is returned. */
    cellularStatus = Cellular_ATHexStrToHex( pLowerCaseString, ppHexData, hexDataLen );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );    
}

/**
 * @brief Test the error path for Cellular_ATHexStrToHex to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATHexStrToHex_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = "";
    uint8_t * const * const ppHexData;
    uint16_t hexDataLen = 0;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATHexStrToHex( pString, ppHexData, hexDataLen );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    pString = CELLULAR_SAMPLE_HEX_STRING;
    hexDataLen = 2;
    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATHexStrToHex( pString, ppHexData, hexDataLen );
    TEST_ASSERT_EQUAL( CELLULAR_AT_NO_MEMORY, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATIsStrDigit to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATIsStrDigit_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString;
    bool * pResult;

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATIsStrDigit( NULL, pResult );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATIsStrDigit( pString, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test happy path for Cellular_ATIsStrDigit to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATIsStrDigit_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char pString[] = CELLULAR_SAMPLE_DIGIT_STRING;
    char pStringWithNull[] = CELLULAR_SAMPLE_DIGIT_STRING_WITH_NULL;
    bool Result;

    /* Check that CELLULAR_AT_SUCCESS is returned. */
    cellularStatus = Cellular_ATIsStrDigit( pString, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
    TEST_ASSERT_EQUAL( true, Result );    

    /* Check that CELLULAR_AT_SUCCESS is returned. */
    cellularStatus = Cellular_ATIsStrDigit( pStringWithNull, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
}

/**
 * @brief Test the error path for Cellular_ATIsStrDigit to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATIsStrDigit_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = "";
    bool Result;

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATIsStrDigit( pString, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATcheckErrorCode to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATcheckErrorCode_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString;
    bool * pResult;
    const char * const * const ppKeyList = {"TEST1:SUCCESS", "TEST2:ERROR"};
    size_t keyListLen = 2;

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATcheckErrorCode( NULL, NULL, keyListLen, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATcheckErrorCode( pString, NULL, keyListLen, pResult );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test happy path for Cellular_ATcheckErrorCode to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATcheckErrorCode_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char pString[] = CELLULAR_SAMPLE_SUCCESS_CODE_STRING;
    char pStringError[] = CELLULAR_SAMPLE_ERROR_CODE_STRING;
    char * ppKeyList[] = {"TEST1:SUCCESS", "TEST2:ERROR"};
    size_t keyListLen = 2;
    bool Result;

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATcheckErrorCode( pString, ppKeyList, keyListLen, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
    TEST_ASSERT_EQUAL( true, Result );

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATcheckErrorCode( pStringError, ppKeyList, keyListLen, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
    TEST_ASSERT_EQUAL( true, Result );
}

/**
 * @brief Test the error path for Cellular_ATcheckErrorCode to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATcheckErrorCode_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = "";
    const char * const * const ppKeyList = {"TEST1:SUCCESS", "TEST2:ERROR"};
    size_t keyListLen = 2;
    bool Result;

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATcheckErrorCode( pString, ppKeyList, keyListLen, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATStrStartWith to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATStrStartWith_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString;
    bool * pResult;
    char * pPrefix;

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATStrStartWith( NULL, NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATStrStartWith( NULL, pPrefix, pResult );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATStrStartWith( pString, NULL, pResult );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATStrStartWith( pString, pPrefix, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

}

/**
 * @brief Test the error path for Cellular_ATStrStartWith to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATStrStartWith_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = "";
    bool * pResult;
    char * pPrefix = "";
    char pStringSuccess[] = CELLULAR_SAMPLE_SUCCESS_CODE_STRING;

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATStrStartWith( pString, pPrefix, pResult );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATStrStartWith( pStringSuccess, pPrefix, pResult );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATStrtoi to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATStrtoi_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    const char * pStr;
    int32_t base;
    int32_t * pResult;

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATStrtoi( NULL, base, pResult );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATStrtoi( pStr, base, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

}

/**
 * @brief Test happy path for Cellular_ATStrtoi to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATStrtoi_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pStr = malloc( sizeof( char ) * ( strlen( CELLULAR_SAMPLE_STRTOL_HAPPY_CASE_STRING ) + 1 ) );
    strcpy( pStr, CELLULAR_SAMPLE_STRTOL_HAPPY_CASE_STRING);
    int32_t base = 10;
    int32_t Result;

    /* Check that CELLULAR_AT_SUCCESS is returned. */
    cellularStatus = Cellular_ATStrtoi( pStr, base, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SAMPLE_STRTOL_HAPPY_CASE_NUMBER, Result );
    free( pStr );
}

/**
 * @brief Test the error path for Cellular_ATStrtoi to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATStrtoi_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pStr = malloc( sizeof( char ) * ( strlen( CELLULAR_SAMPLE_STRTOL_ERROR_CASE_STRING ) + 1 ) );
    strcpy( pStr, CELLULAR_SAMPLE_STRTOL_ERROR_CASE_STRING);
    int32_t base = 10;
    int32_t Result;

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATStrtoi( pStr, base, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_ERROR, cellularStatus );
    free(pStr);
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATStrDup to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATStrDup_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pDst;
    char * pSrc;

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATStrDup( NULL, pSrc );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATStrDup( pDst, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

}

/**
 * @brief Test happy path for Cellular_ATStrDup to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATStrDup_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pDst;
    char pSrc[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

    /* Check that CELLULAR_AT_SUCCESS is returned. */
    cellularStatus = Cellular_ATStrDup( &pDst, pSrc );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
}

/**
 * @brief Test that any NULL parameter causes Cellular_ATIsPrefixPresent  to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATIsPrefixPresent_Invalid_Param( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString;
    bool * pResult;

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATIsPrefixPresent ( NULL, pResult );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATIsPrefixPresent ( pString, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

}

/**
 * @brief Test happy path for Cellular_ATIsPrefixPresent to return CELLULAR_AT_SUCCESS and got expected results.
 */
void test_Cellular_ATIsPrefixPresent_Happy_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pStr = malloc( sizeof( char ) * ( strlen( CELLULAR_SAMPLE_PREFIX_STRING_INPUT ) + 1 ) );
    strcpy( pStr, CELLULAR_SAMPLE_PREFIX_STRING_INPUT);
    bool Result;

    /* Check that CELLULAR_AT_SUCCESS is returned. */
    cellularStatus = Cellular_ATIsPrefixPresent( pStr, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
    TEST_ASSERT_EQUAL( true, Result );
    free( pStr );
}

/**
 * @brief Test the error path for Cellular_ATIsPrefixPresent to return CELLULAR_AT_BAD_PARAMETER.
 */
void test_Cellular_ATIsPrefixPresent_Error_Path( void )
{
    CellularATError_t cellularStatus = CELLULAR_AT_SUCCESS;
    char * pString = "";
    bool Result;

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATIsPrefixPresent( pString, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_BAD_PARAMETER, cellularStatus );

    pString = malloc( sizeof( char ) * ( strlen( CELLULAR_SAMPLE_PREFIX_STRING_WRONG_INPUT ) + 1 ) );
    strcpy( pString, CELLULAR_SAMPLE_PREFIX_STRING_WRONG_INPUT);

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATIsPrefixPresent( pString, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
    TEST_ASSERT_EQUAL( false, Result );
    free( pString );

    pString = malloc( sizeof( char ) * ( strlen( CELLULAR_SAMPLE_PREFIX_STRING_STAR_FIRST_INPUT ) + 1 ) );
    strcpy( pString, CELLULAR_SAMPLE_PREFIX_STRING_STAR_FIRST_INPUT);

    /* Check that CELLULAR_AT_BAD_PARAMETER is returned. */
    cellularStatus = Cellular_ATIsPrefixPresent( pString, &Result );
    TEST_ASSERT_EQUAL( CELLULAR_AT_SUCCESS, cellularStatus );
    TEST_ASSERT_EQUAL( false, Result );
    free( pString );
    
}