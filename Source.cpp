/*
JPEG File Renamer
Copyright Berkm125
*/

//All the needed libraries don't remove or it will not work.
#include <windows.h>
#include <gdiplus.h>
#include <strsafe.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <dirent.h> //You need to install this library unless you run Linux/UNIX.

//Define constants and link needed libraries
#define MAX_PROPTYPE_SIZE 30 
#define DATE_TAKEN_NUM 17
#pragma comment(lib,"gdiplus.lib")
#pragma comment(lib, "User32.lib")
//Include these namespaces to make namespace-based calls way simpler.
using namespace Gdiplus;
using namespace std;
//Function that we will need later on in order to convert the property types within the extracted metadata into a WCHAR string.
HRESULT PropertyTypeFromWORD(WORD index, WCHAR* string, UINT maxChars)
{
    HRESULT hr = E_FAIL;

    const wchar_t* propertyTypes[] = {
       L"Nothing",                   // 0
       L"PropertyTagTypeByte",       // 1
       L"PropertyTagTypeASCII",      // 2
       L"PropertyTagTypeShort",      // 3
       L"PropertyTagTypeLong",       // 4
       L"PropertyTagTypeRational",   // 5
       L"Nothing",                   // 6
       L"PropertyTagTypeUndefined",  // 7
       L"Nothing",                   // 8
       L"PropertyTagTypeSLONG",      // 9
       L"PropertyTagTypeSRational" }; // 10

    hr = StringCchCopyW(string, maxChars, propertyTypes[index]);
    return hr;
}

/*Function that we will invoke later to remove whitespaces and colons from the filename,
to instead replace those colons and whitespaces with underscores ('_') and a 'T' to seperate
the time and the date.
*/
void removeSpaces(char* str)
{
    for (int i = 0; str[i]; i++) {
        if (str[i] == ' ') {
            str[i] = 'T';
        }
        if (str[i] == ':') {
            str[i] = '_';
        }
    }
}

//Look in the main() loop.
void mainFunc(char* filename, char md) {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    //Variables
    FILE* fp;
    UINT  size = 0;
    UINT  count = 0;
    char newfilename[32];
    wchar_t wfilename[32];
    WCHAR strPropertyType[MAX_PROPTYPE_SIZE] = L"";

    //Microsoft garbage likes to use wchar_t instead of char, so we have to invoke mbstowcs to correctly 
    mbstowcs(wfilename, filename, strlen(filename) + 1);

    //BEFORE DOING ANYTHING check if the file is valid using *fp, so that we prevent the application from crashing if the file is invalid.
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("File does not exist, returning\n");
        free(fp);
        return;
    }
    fclose(fp);
    //Create the Image object, get the property size and the amount of metadata within the image file.
    Image* img = new Image(wfilename);
    img->GetPropertySize(&size, &count);
    printf("There are %d pieces of metadata in the file.\n\n", count);

    //Create a PropertyItem buffer object which allows you to hold all of the property items in the metadata in an array
    PropertyItem* pPropBuffer = (PropertyItem*)malloc(size);

    // Get the array of PropertyItem objects.
    img->GetAllPropertyItems(size, count, pPropBuffer);

    //Get all the metadata information
    if (md == 'Y' || md == 'y') {
        for (UINT j = 0; j < count; ++j)
        {
            // Convert the property type from a WORD to a string.
            PropertyTypeFromWORD(
                pPropBuffer[j].type, strPropertyType, MAX_PROPTYPE_SIZE);

            printf("Property Item %d\n", j);
            printf("  id: 0x%x\n", pPropBuffer[j].id);
            wprintf(L"  type: %s\n", strPropertyType);
            printf("  length: %d bytes\n", pPropBuffer[j].length);
            printf("  The equipment make is %s.\n\n", pPropBuffer[j].value);
        }
    }

    //Print the image timestamp
    PropertyTypeFromWORD(pPropBuffer[DATE_TAKEN_NUM].type, strPropertyType, MAX_PROPTYPE_SIZE);
    printf("The timestamp of when the photo was taken is %s.\n", pPropBuffer[DATE_TAKEN_NUM].value);
    
    //Transfer the timestamp data (in 'string' format) into newfilename[]
    sprintf(newfilename, "%s", (char*)pPropBuffer[DATE_TAKEN_NUM].value);
    
    //Remove whitespaces and colons from the file name, and add underscores to replace them and a T to seperate date and time
    removeSpaces(newfilename);
    
    //Oh and add a '.jpg' to the end to specify the file extension
    strcat(newfilename, ".jpg");
    
    //Print the new name of the JPEG file
    printf("New file name: %s\n", newfilename);
    
    /*Delete the img object in order to close the file (because loading it into an Image automatically opens it)
    because if you don't then you cannot gain file access.
    */
    delete img;

    //Rename the file, return a detailed error message using errno.h
    if (rename(filename, newfilename) != 0)
        cout << "Error: " << strerror(errno) << endl;
    
    //Cleanup:
    free(pPropBuffer);
    GdiplusShutdown(gdiplusToken);
    return;
}

//This is how we access all the files in the directory
char** directorySweep(void) {
    struct dirent* de;
    char md = 'N';
    int sz = 0;
    int index = 0;
    char* df;
    //Create a DIR object and open the current directory
    DIR* dr = opendir(".");
    //Terminate if it's an invalid directory
    if (dr == NULL) {
        printf("Could not open current directory");
        return 0;
    }
    //Get the names of all the files in the directory and pass them to the mainFunc function.
    while ((de = readdir(dr)) != NULL) {
        printf("%s\n", de->d_name);
        //This is for JPEGs, however, so we have to make sure that the file is indeed a JPEG before we pass it to mainFunc for renaming.
        if(strstr(de->d_name, ".jpg") != NULL || strstr(de->d_name, ".JPG") != NULL)
            mainFunc(de->d_name, md);
        //Increment sz
        sz++;
    }
    //Create a dynamic string array to hold all of the file names
    df = (char*)malloc(32 * sz * sizeof(char));
    //Re read all of the files in the directory and push them to the array
    while ((de = readdir(dr)) != NULL) {
        sprintf((df + index * sz), "%s", de->d_name);
        index++;
    }
    //Close the directory
    closedir(dr);
    //Return the list of files in case you want to use it later.
    return &df;
}

//main
int main(void) {
    char filename[32];
    char md;
    //Conduct a directory sweep
    directorySweep();
    //Ask for file name
    printf("Now do this for any other JPEGs.\n");
    printf("Enter file name: ");
    scanf(" %s", filename);

    //Ask for metadata
    printf("Would you like to see all JPEG metadata? (Y/N): ");
    scanf(" %c", &md);
}
