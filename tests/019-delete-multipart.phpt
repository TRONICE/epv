--TEST--
EPV: DELETE request with multipart/form-data body
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--CGI--
--ENV--
REQUEST_METHOD=DELETE
CONTENT_TYPE=multipart/form-data; boundary=testboundary
--POST_RAW--
--testboundary
Content-Disposition: form-data; name="id"

99
--testboundary--
--FILE--
<?php
var_dump($_DELETE);
?>
--EXPECT--
array(1) {
  ["id"]=>
  string(2) "99"
}
