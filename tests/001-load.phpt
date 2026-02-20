--TEST--
EPV: extension loads and registers superglobals
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--FILE--
<?php
var_dump(extension_loaded('epv'));

// Superglobals must exist and be accessible
var_dump(isset($_PUT));
var_dump(isset($_DELETE));
var_dump(isset($_PATCH));
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
