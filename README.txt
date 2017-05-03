Project Partners: Thomas Flanagan and David Mattia

I talked to professor striegel a few times over the course of the project and at various points that I could make a few design decisions.

Notably, we do not check ahead of time to see if there is enough room for a file, we terminate partially written if necessary.

         we use a bitmap and associated size to tell if format/mount is allowed

         we do not automatically mount a preloaded disk, so ./simplefs image.20 20 is allowed to be formatted.

         we did not recover from terminations halfway thorugh writing as that functionality was below the fs level.

Oh, and we added a command "bitmap" to the shell to print out the bitmap
