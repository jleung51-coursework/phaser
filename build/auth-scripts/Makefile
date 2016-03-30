# TODO: adduser.sh, authtable.sh

GREYDARK = \033[1;30m
CYAN = \033[0;36m
GREEN = \033[0;32m
NOCOLOR = \033[0m

help:
	@echo "$(GREEN)INSTRUCTIONS:"
	@echo ""
	@echo "  Before running any of these instructions, set the necessary environment"
	@echo "    variables by running: $$ source setvars.sh"
	@echo ""
	@echo "  To create and set up table DataTable with:"
	@echo "    one partition named \"authenticated_partition\""
	@echo "    one row named \"authenticated_row\" with the properties:"
	@echo "      Prop1: \"property1\""
	@echo "      Prop2: \"property2\""
	@echo "    run: $$ make datasetup"
	@echo ""
	@echo "  To create and set up table AuthTable with:"
	@echo "    one partition named \"Userid\""
	@echo "    one row named \"username\" with the properties:"
	@echo "      Password: \"password\""
	@echo "      DataPartition: \"authenticated_partition\""
	@echo "      DataRow: \"authenticated_row\""
	@echo "    run: $$ make authsetup"
	@echo ""
	@echo "  To create an authentication token, run: $$ make token"
	@echo "  To use the authentication token to read the contents of table DataTable,"
	@echo "    run: $$ make authread"
	@echo ""
	@echo "  To show the contents of table DataTable, run: $$ make dumpdatatable"
	@echo "  To show the contents of table AuthTable, run: $$ make dumpauthtable"
	@echo ""
	@echo "  To delete the entity previously set up with datasetup,"
	@echo "    run: $$ make dataclean"
	@echo "  To delete the entity previously set up with authsetup,"
	@echo "    run: $$ make authclean"
	@echo ""
	@echo "  To reset all shell variables, run: $$ make clean"
	@echo "  To remove AuthTable and DataTable (only when absolutely"
	@echo "    necessary), run: $$ make tableclean $(NOCOLOR)"

datasetup:
	curl -iX post $B/CreateTableAdmin/DataTable
	curl -iX put -H "$H" -d '{"Prop1":"property1", "Prop2":"property2"}' $B/UpdateEntityAdmin/DataTable/authenticated_partition/authenticated_row

adduser:
	chmod u+x ./adduser.sh
	./adduser.sh username password authenticated_partition authenticated_row

authsetup:
	curl -iX post $B/CreateTableAdmin/AuthTable
	make adduser

token:
	@echo "$(GREEN)To create the token, run $$ source tr.sh $(NOCOLOR)"

authread:
	chmod u+x ./readauth.bash
	./readauth.bash
	@echo "$(GREEN)You may be seeing an error because ReadEntityAuth has not been implemented yet. $(NOCOLOR)"

dumpdatatable:
	curl -iX get $B/ReadEntityAdmin/DataTable
	@echo ""

# dumptable.bash does not work at the moment because ReadEntityAuth has not been implemented
#	./dumptable.bash

dumpauthtable:
	chmod u+x ./authtable.sh
	./authtable.sh

dataclean:
	curl -iX delete $B/DeleteEntityAdmin/DataTable/authenticated_partition/authenticated_row
	@echo "$(GREEN)If the status code was 500: Internal Error, then the entity"
	@echo "has most likely already been deleted. $(NOCOLOR)"

authclean:
	curl -iX delete $B/DeleteEntityAdmin/AuthTable/Userid/username
	@echo "$(GREEN)If the status code was 500: Internal Error, then the entity"
	@echo "has most likely already been deleted. $(NOCOLOR)"

clean:
	chmod u+x ./cleanvars.sh
	./cleanvars.sh

tableclean:
	curl -iX delete $B/DeleteTableAdmin/AuthTable
	curl -iX delete $B/DeleteTableAdmin/DataTable
