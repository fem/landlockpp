LandlockPP Linux Landlock API Wrapper for C++
=============================================

This library provides a simple and easy-to-use wrapper for the Linux Landlock API.
It provides best-effort security by enforcing only Landlock rules which
were available in the API at compile time and are available in the Kernel ABI at runtime.

## Usage Example

The `RulesetTest.cpp` file shows an up-to-date version of how to use the library.
Call the library as early as possible in your code.

First, create a `landlock::Ruleset`, specifying all the actions which should be filtered by Landlock.
Supported actions are predefined in the `landlock::action` namespace
and have names corresponding to the names of the Landlock C API.
Example:

```cpp
Ruleset ruleset{
    {landlock::action::FS_READ_FILE,
     landlock::action::FS_READ_DIR,
     landlock::action::FS_WRITE_FILE,
     landlock::action::FS_TRUNCATE,
     landlock::action::FS_EXECUTE}
};
```

When specifying actions, there is no need to take care of checking the API or ABI version.
LandlockPP takes care of calculating the maximum compatible set of actions
depending on the API and ABI version at compile time and runtime.

After creating the ruleset, add rules as needed.
LandlockPP takes care of creating necessary file descriptors and other stuff necessary,
so users of the library only need to specify paths etc.
Multiple paths/ports/etc. (whatever the object of the rule)
can be specified and lead to multiple Landlock rules being generated,
so the specified actions are allowed for all specified objects
(i.e. a cross product of all specified actions and objects is allowed).
Example:


```cpp
landlock::PathBeneathRule rule;
rule
    .add_path("/first/path/to/file.txt")
    .add_path("/second/path/to/otherfile.txt")
    .add_action(landlock::action::FS_READ_FILE)
    .add_action(landlock::action::FS_WRITE_FILE)
    .add_action(landlock::action::FS_TRUNCATE);
ruleset.add_rule(std::move(rule));
```

This grants read, write and truncate permissions for the files `/first/path/to/file.txt` and `/second/path/to/otherfile.txt`.

After all rules have been added, the rules need to be enforced:

```cpp
ruleset.enforce();
```

Afterwards, no new permissions can be obtained by the running process and Landlock rules are properly enforced
within the limits of what the running Kernel supports.
If the Kernel does not support Landlock at all, nothing is enforced (as this library is meant for best-effort security).
In this case, `Ruleset::landlock_enabled()` returns `false`, so library consumers can handle this case (e.g. by printing a warning).

## License

Copyright (C) 2024 Forschungsgemeinschaft elektronische Medien e.V.

This library is licensed under the terms of the GNU General Public License Version 3.
See LICENSE.txt for the license text.
