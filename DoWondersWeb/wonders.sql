SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";

CREATE TABLE `enums` (
  `enum_id` int(11) NOT NULL,
  `name` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `type_id` int(11) NOT NULL,
  `file` varchar(256) COLLATE utf8_unicode_ci NOT NULL,
  `lineno` mediumint(9) NOT NULL,
  `include` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `definition` varchar(512) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`enum_id`),
  UNIQUE KEY `enum_id` (`enum_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE `enum_items` (
  `enum_id` int(11) NOT NULL,
  `item_index` smallint(6) NOT NULL,
  `item_name` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `int_value` int(11) DEFAULT NULL,
  PRIMARY KEY (`enum_id`,`item_index`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE `functions` (
  `name` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `var_id` int(11) NOT NULL,
  `type_id` int(11) NOT NULL,
  `file` varchar(256) COLLATE utf8_unicode_ci NOT NULL,
  `lineno` mediumint(9) NOT NULL,
  `include` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `dll` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `definition` varchar(512) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`name`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE `func_types` (
  `func_id` int(11) NOT NULL,
  `convention` tinyint(4) NOT NULL,
  `return_type_id` int(11) NOT NULL,
  `paramlist_id` int(11) NOT NULL,
  `ellipsis` tinyint(1) NOT NULL,
  `file` varchar(256) COLLATE utf8_unicode_ci NOT NULL,
  `lineno` mediumint(9) NOT NULL,
  `include` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `definition` varchar(512) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`func_id`),
  UNIQUE KEY `func_id` (`func_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE `params` (
  `paramlist_id` int(11) NOT NULL,
  `param_index` smallint(6) NOT NULL,
  `type_id` int(11) NOT NULL,
  `name` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `definition` varchar(512) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`paramlist_id`,`param_index`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE `structures` (
  `struct_id` int(11) NOT NULL,
  `type_id` int(11) NOT NULL,
  `internal_name` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `canonical_name` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `is_struct` tinyint(1) NOT NULL,
  `pack` smallint(6) NOT NULL,
  `align` smallint(6) NOT NULL,
  `alignas` smallint(6) NOT NULL,
  `alignas_explicit` tinyint(1) NOT NULL,
  `is_complete` tinyint(1) NOT NULL,
  `file` varchar(256) COLLATE utf8_unicode_ci NOT NULL,
  `lineno` mediumint(9) NOT NULL,
  `include` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `definition` varchar(512) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`struct_id`),
  UNIQUE KEY `struct_id` (`struct_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE `struct_members` (
  `struct_id` int(11) NOT NULL,
  `member_index` smallint(6) NOT NULL,
  `type_id` int(11) NOT NULL,
  `name` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `bit_offset` smallint(6) NOT NULL,
  `bits` smallint(6) NOT NULL,
  `definition` varchar(512) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`struct_id`,`member_index`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE `types` (
  `type_id` int(11) NOT NULL,
  `name` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `flags` int(11) unsigned NOT NULL,
  `sub_id` int(11) NOT NULL,
  `count` smallint(6) NOT NULL,
  `size` smallint(6) NOT NULL,
  `align` smallint(6) NOT NULL,
  `alignas` smallint(6) NOT NULL,
  `alignas_explicit` tinyint(1) NOT NULL,
  `file` varchar(256) COLLATE utf8_unicode_ci NOT NULL,
  `lineno` mediumint(9) NOT NULL,
  `include` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `definition` varchar(512) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`type_id`),
  UNIQUE KEY `type_id` (`type_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE `vars` (
  `var_id` int(11) NOT NULL,
  `type_id` int(11) NOT NULL,
  `name` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `int_value` int(11) DEFAULT NULL,
  `file` varchar(256) COLLATE utf8_unicode_ci NOT NULL,
  `lineno` mediumint(9) NOT NULL,
  `include` varchar(64) COLLATE utf8_unicode_ci NOT NULL,
  `definition` varchar(512) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`var_id`),
  UNIQUE KEY `var_id` (`var_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
