--TEST--
EPV: PATCH request with multipart/form-data body
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--CGI--
--ENV--
REQUEST_METHOD=PATCH
CONTENT_TYPE=multipart/form-data; boundary=testboundary
--POST_RAW--
--testboundary
Content-Disposition: form-data; name="status"

active
--testboundary--
--FILE--
<?php
var_dump($_PATCH);
?>
--EXPECT--
array(1) {
  ["status"]=>
  string(6) "active"
}
