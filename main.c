#include <assert.h>
#include <mongoc/mongoc.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <bson.h>
#include <stdio.h>
#include <locale.h>
#include <memory.h>
#include "tester/testers.h"
int
main (int argc, char *argv[])
{
    mongoc_gridfs_t *gridfs;
    mongoc_client_t *client;
    const char *uri_string = "mongodb://127.0.0.1:27017/?appname=gridfs-example";
    mongoc_uri_t *uri;
    bson_error_t error;
    //1. prepare one mongoDB database
    mongoc_init ();
    uri = mongoc_uri_new_with_error (uri_string, &error);
    if (!uri) {
        return EXIT_FAILURE;
    }
    client = mongoc_client_new_from_uri (uri);
    assert (client);
    mongoc_client_set_error_api (client, 2);
    gridfs = mongoc_client_get_gridfs (client, "test", "fs", &error);
    //add test case at following

    tester(gridfs);
    mongoc_gridfs_destroy (gridfs);
    mongoc_uri_destroy (uri);
    mongoc_client_destroy (client);
    mongoc_cleanup ();
    assert(0);

    return EXIT_SUCCESS;
}
