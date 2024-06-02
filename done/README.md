![Logo](https://upload.wikimedia.org/wikipedia/commons/f/f4/Logo_EPFL.svg)

# CS202 Project : ImgFS

1. What we did and did not do in the project

For the project, we fully completed the material up until week 12 included (HTTP Layer).    
The last week (Webserver services) is partially complete: the JSON part has been done, as well as the part concerning the HTTP version of the ImgFS commands.
The part about multi-threading has not been completed. We gave it a try but we sadly couldn't manage to make it work in time.

2. Any particular remark about our project

A few HTTP unit tests have been written in `done/tests/unit/unit-test-http`, you can find them under *CUSTOM TESTS* comment at the beginning of the file.

The `static_unless_test` keywords have been removed from the *http_parse_header* and *get_next_token* functions in `http_prot.c` to make sure nothing would go wrong when testing on your side since we had to make some tweaks to make it work, as stated in [this Ed post (#1550)](https://edstem.org/eu/courses/1124/discussion/111071).


3. Anything else we would like to say about the project

Nothing particular !