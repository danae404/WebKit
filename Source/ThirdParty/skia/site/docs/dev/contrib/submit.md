---
title: 'How to submit a patch'
linkTitle: 'How to submit a patch'
---

## Configure git

<!--?prettify lang=sh?-->

    git config --global user.name "Your Name"
    git config --global user.email you@example.com

## Making changes

First create a branch for your changes:

<!--?prettify lang=sh?-->

    git config branch.autosetuprebase always
    git checkout -b my_feature origin/main

After making your changes, create a commit

<!--?prettify lang=sh?-->

    git add [file1] [file2] ...
    git commit

If your branch gets out of date, you will need to update it:

<!--?prettify lang=sh?-->

    git pull
    python3 tools/git-sync-deps

## Adding a unit test

If you are willing to change Skia codebase, it's nice to add a test at the same
time. Skia has a simple unittest framework so you can add a case to it.

Test code is located under the 'tests' directory.

See [Writing Unit and Rendering Tests](/docs/dev/testing/tests) for details.

Unit tests are best, but if your change touches rendering and you can't think of
an automated way to verify the results, consider writing a GM test. Also, if
your change is in the GPU code, you may not be able to write it as part of the
standard unit test suite, but there are GPU-specific testing paths you can
extend.

## Updating BUILD.bazel files

If you added or removed files, you will need to update the `BUILD.bazel` file in the directory
of those files. Many `BUILD.bazel` files have a list of files that is broken up into two
[`filegroup`](https://bazel.build/reference/be/general#filegroup) rules using the
`split_srcs_and_hdrs` macro. You should add the new file names or delete the old ones from these
file lists.

If your feature will be conditionally enabled (e.g. like the GPU backends or image codecs), you
may need to add or modify
[`select`](https://bazel.build/reference/be/common-definitions#configurable-attributes) statements
to achieve that goal. Look at existing rules for examples of this.

## Submitting a patch

For your code to be accepted into the codebase, you must complete the
[Individual Contributor License Agreement](http://code.google.com/legal/individual-cla-v1.0.html).
You can do this online, and it only takes a minute. If you are contributing on
behalf of a corporation, you must fill out the
[Corporate Contributor License Agreement](http://code.google.com/legal/corporate-cla-v1.0.html)
and send it to us as described on that page. Add your (or your organization's)
name and contact info to the AUTHORS file as a part of your CL.

Now that you've made a change and written a test for it, it's ready for the code
review! Submit a patch and getting it reviewed is fairly easy with depot tools.

Use `git-cl`, which comes with
[depot tools](http://sites.google.com/a/chromium.org/dev/developers/how-tos/install-depot-tools).
For help, run `git cl help`. Note that in order for `git cl` to work correctly,
it needs to run on a clone of <https://skia.googlesource.com/skia>. Using clones
of mirrors, including Google's mirror on GitHub, might lead to issues with
`git cl` usage.

### Find a reviewer

Ideally, the reviewer is someone who is familiar with the area of code you are
touching. Look at the git blame for the file to see who else has been editing
it. If unsuccessful, another option is to click on the 'Suggested Reviewers'
button to add one of the listed Skia contacts. They should be able to add
appropriate reviewers for your change. The button is located here:
<img src="/docs/dev/contrib/SuggestedReviewers.png" style="display: inline-block; max-width: 75%" />

### Uploading changes for review

Skia uses the Gerrit code review tool. Skia's instance is
[skia-review](http://skia-review.googlesource.com). Use `git cl` to upload your
change:

<!--?prettify lang=sh?-->

    git cl upload

You may have to enter a Google Account username and password to authenticate
yourself to Gerrit. A free gmail account will do fine, or any other type of
Google account. It does not have to match the email address you configured using
`git config --global user.email` above, but it can.

The command output should include a URL, similar to
([https://skia-review.googlesource.com/c/4559/](https://skia-review.googlesource.com/c/4559/)),
indicating where your changelist can be reviewed.

### Submit try jobs

Skia's trybots allow testing and verification of changes before they land in the
repo. You need to have permission to trigger try jobs; if you need permission,
ask a committer. After uploading your CL to
[Gerrit](https://skia-review.googlesource.com/), you may trigger a try job for
any job listed in tasks.json, either via the Gerrit UI, using `git cl try`, eg.

    git cl try -B skia.primary -b Some-Tryjob-Name

or using bin/try, a small wrapper for `git cl try` which helps to choose try
jobs. From a Skia checkout:

    bin/try --list

You can also search using regular expressions:

    bin/try "Test.*Pixel.*Release"

For more information about testing, see
[testing infrastructure](/docs/dev/testing/automated_testing).

### Request review

Go to the supplied URL or go to the code review page and select the **Your**
dropdown and click on **Changes**. Select the change you want to submit for
review and click **Reply**. Enter at least one reviewer's email address. Now add
any optional notes, and send your change off for review by clicking on **Send**.
Unless you send your change to reviewers, no one will know to look at it.

_Note_: If you don't see editing commands on the review page, click **Sign in**
in the upper right. _Hint_: You can add -r reviewer@example.com --send-mail to
send the email directly when uploading a change using `git-cl`.

## The review process

If you submit a giant patch, or do a bunch of work without discussing it with
the relevant people, you may have a hard time convincing anyone to review it!

Code reviews are an important part of the engineering process. The reviewer will
almost always have suggestions or style fixes for you, and it's important not to
take such suggestions personally or as a commentary on your abilities or ideas.
This is a process where we work together to make sure that the highest quality
code gets submitted!

You will likely get email back from the reviewer with comments. Fix these and
update the patch set in the issue by uploading again. The upload will explain
that it is updating the current CL and ask you for a message explaining the
change. Be sure to respond to all comments before you request review of an
update.

If you need to update code the code on an already uploaded CL, simply edit the
code, commit it again locally, and then run git cl upload again e.g.

    echo "GOATS" > whitespace.txt
    git add whitespace.txt
    git commit -m 'add GOATS fix to whitespace.txt'
    git cl upload

Once you're ready for another review, use **Reply** again to send another
notification (it is helpful to tell the reviewer what you did with respect to
each of their comments). When the reviewer is happy with your patch, they will
approve your change by setting the Code-Review label to "+1".

_Note_: As you work through the review process, both you and your reviewers
should converse using the code review interface, and send notes.

Once your change has received an approval, you can click the "Submit to CQ"
button on the codereview page and it will be committed on your behalf.

Once your commit has gone in, you should delete the branch containing your
change:

    git checkout -q origin/main
    git branch -D my_feature

## Final Testing

Skia's principal downstream user is Chromium, and any change to Skia rendering
output can break Chromium. If your change alters rendering in any way, you are
expected to test for and alleviate this. You may be able to find a Skia team
member to help you, but the onus remains on each individual contributor to avoid
breaking Chrome.

### Evaluating Impact on Chromium

Keep in mind that Skia is rolled daily into Blink and Chromium. Run local tests
and watch canary bots for results to ensure no impact. If you are submitting
changes that will impact layout tests, follow the guides below and/or work with
your friendly Skia-Blink engineer to evaluate, rebaseline, and land your
changes.

Resources:

[How to land Skia changes that change Blink layout test results](/docs/dev/chrome/blink/)

If you're changing the Skia API, you may need to make an associated change in
Chromium. If you do, please follow these instructions:
[Landing Skia changes which require Chrome changes](/docs/dev/chrome/changes/)

## Check in your changes

### Non-Skia-committers

If you already have committer rights, you can follow the directions below to
commit your change directly to Skia's repository.

If you don't have committer rights in https://skia.googlesource.com/skia.git ...
first of all, thanks for submitting your patch! We really appreciate these
submissions. After receiving an approval from a committer, you will be able to
click the "Submit to CQ" button and submit your patch via the commit queue.

In special instances, a Skia committer may assist you in landing the change by
uploading a new codereview containing your patch (perhaps with some small
adjustments at their discretion). If so, you can mark your change as
"Abandoned", and update it with a link to the new codereview.

### Skia committers

- tips on how to apply an externally provided patch are [here](../patch)
- when landing externally contributed patches, please note the original
  contributor's identity (and provide a link to the original codereview) in the
  commit message

  `git-cl` will squash all your commits into a single one with the description
  you used when you uploaded your change.

  ```
  git cl land
  ```

  or

  ```
  git cl land -c 'Contributor Name <email@example.com>'
  ```
