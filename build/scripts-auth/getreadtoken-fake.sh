#!/bin/bash
# Generate a fake token for testing when you don't yet have a working authentication server
# Useful for testing shell scripts.
# Unfortunately, this token cannot be used with Azure Tables
echo -n '{"token":"sv=2015-04-05&sig=5GgaDDWLjMPoTBA7gP34CyeKLqPnqHRlnKdelVxtuG0%3D&spr=https%2Chttp&se=2016-03-23T22%3A57%3A49Z&sp=r&tn=DataTable&spk=USA&srk=Franklin%2CAretha&epk=USA&erk=Franklin%2CAretha"}'