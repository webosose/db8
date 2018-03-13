##### file-name=libdump.sh, file-mode=755
#!/bin/sh
# NO GLOBAL VARIABLES USE

set -e

dump_make_one() {
	local dumps_dir="${1}"
	local entries=${2}
	local gtest="$(gtest_get_location)"

	${gtest} --gtest_print_time=0 --nsteps=$entries --dump-to="${dumps_dir}/dump-$entries.txt"
}

dump_main() {
	local dumps_dir="${1}"
	local start=${2}
	local end=${3}
	local step=${4}
	local work_dir=${5}
	local msg=

	if ! [ -d "${dumps_dir}" ]; then
		msg="$(mkdir -p "${dumps_dir}")" || \
			die "${FUNCNAME}($dumps_dir} ${start} ${end} ${step}): ${msg}"
	fi

	for entries in $(seq ${start} ${step} ${end}); do
		if [ -f ${dumps_dir}/dump-${entries}.txt ]; then
			warning "File exists: ${dumps_dir}/dump-${entries}.txt"
		else
			inform "Dumping file: ${dumps_dir}/dump-${entries}.txt"
			dump_make_one "${dumps_dir}" ${entries}
		fi
	done
}
##### file-name=libplot.sh, file-mode=755
#!/bin/sh
# NO GLOBAL VARIABLES USE
set -e

plot_check_gnuplot_version() {
	if [ "$(gnuplot --version)" = "" ]; then
			die "Gnuplot is not installed"
	fi
}

plot() {
	local test_result_file=$1
	local plot_label=$2
	local cmd=

	plot_check_gnuplot_version

	cmd="set term png; \
	set xlabel '# of records'; \
	set output \"cpu_out.png\"; \
	set title \"${plot_label}\"; \
	set ylabel 'Cpu time usage, seconds'; \
	plot \"${test_result_file}\" using 1:2 title 'cpu_ktime' w linespoints, \
			 \"${test_result_file}\" using 1:3 title 'cpu_utime' w linespoints;"

	echo ${cmd} | gnuplot

	cmd="set term png; \
	set xlabel '# of records'; \
	set output \"mem_out.png\"; \
	set title \"${plot_label}\"; \

	set ylabel 'Memory usage, Kb'; \
	plot \"${test_result_file}\" using 1:4 title 'mem_avg' w linespoints, \
			 \"${test_result_file}\" using 1:5 title 'mem_peak' w linespoints;"

	echo ${cmd} | gnuplot
}

plot_total() {
	local test_result_dirs="${@}"
	local cmd=

	plot_check_gnuplot_version

	cmd="set term png; \
	set output \"total_mem.png\"; \
	set title \"Memory consumption of all tests\"; \
	set xlabel '# of records'; \
	set ylabel 'Memory usage, Kb'; \
	plot for [file in \"${test_result_dirs}\"] file.'/out.txt' using 1:4 title file.'_mem_avg' with linespoints, \
		for [file in \"${test_result_dirs}\"] file.'/out.txt' using 1:5 title file.'_mem_peak' with linespoints;"

	echo ${cmd} | gnuplot || \
		die "${FUNCNAME}: gnuplot failed"

	cmd="set term png; \
	set output \"total_cpu.png\"; \
	set title \"Cpu usage of all tests\"; \
	set xlabel '# of records'; \
	set ylabel 'Cpu time usage, seconds'; \
	plot for [file in \"${test_result_dirs}\"] file.'/out.txt' using 1:2 title file.'_ktime' with linespoints, \
		for [file in \"${test_result_dirs}\"] file.'/out.txt' using 1:3 title file.'_utime' with linespoints;"

	echo ${cmd} | gnuplot || \
		die "${FUNCNAME}: gnuplot failed"
}

plot_main() {
	local all_tests="${@}"

	for testname in ${all_tests}; do
		cd "${testname}" || \
			die "cd(${testname}): failed"
		if ! [ -f "$(pwd)/out.txt" ]; then
			die "${FUNCNAME}: $(pwd)/out.txt: No such file"
		fi

		if [ "$(head -n1 $(pwd)/out.txt)" != "# dbclient-perf-data-1" ]; then
			die "${FUNCNAME}: $(pwd)/out.txt: No 'dbclient-perf-data-1' signature";
		fi

		inform "Plotting $(pwd)/out.txt"
		plot "out.txt" "${testname}"
		cd ..
	done

	inform "Plotting overall results of: ${all_tests}"
	plot_total "${all_tests}"
}
##### file-name=libutil.sh, file-mode=755
#!/bin/sh
# NO GLOBAL VARIABLES USE

set -e

die()     { echo "$(_fmt error "${@}")"; exit $?; }
inform()  { echo "$(_fmt inform "${@}")"; }
warning() {	echo "$(_fmt warning "${@}")"; }
debug()   { if [ "${LOG_LEVEL}" -ge 2 ]; then echo "$(_fmt debug "${@}")"; fi }

_fmt () {
	local color_warning="\x1b[33m"
	local color_ok="\x1b[32m"
	local color_bad="\x1b[31m"
	local color="${color_bad}"
	local severity=${1}
	shift
	local msg="${@}"

	if [ "${severity}" = "inform" ]; then
		color="${color_ok}"
	elif [ "${severity}" = "debug" ]; then
		color=""
	elif [ "${severity}" = "warning" ]; then
		color="${color_warning}"
	elif [ "${severity}" = "error" ]; then
		color="${color_bad}"
	else
		color=""
	fi

	local color_reset="\x1b[0m"
	if [[ "${TERM}" != "xterm"* ]] || [ -t 1 ]; then
		# Don't use colors on pipes or non-recognized terminals
		color=""
		color_reset=""
	fi

	echo -e "${color}[${severity}]: ${msg}${color_reset}";
}
##### file-name=libssh.sh, file-mode=755
#!/bin/sh
# NO GLOBAL VARIABLES USE

set -e

ssh_copy_id_with_timeout() {
	local ssh_args="${1}"
	local timeout="${2}"
	local ret_code=0
	local msg=

	msg="$(timeout --kill-after=${timeout} ${timeout} ssh-copy-id ${ssh_args} 2>&1)" || ret_code=$?
	if [[ ${ret_code} == 124 ]]; then
		# 'timeout' command returns 124 in case if command
		# actually timed out. Not an error condition
		warning "${FUNCNAME}(${ssh_args} ${timeout}): timed out, probably you will have to enter a password manually"
	elif [[ ${ret_code} != 0 ]]; then
		die "${FUNCNAME}(${ssh_args} ${timeout}): ${msg}"
	fi
}

ssh_exec_cmd_remote() {
	local ssh_args="${1}"
	local remote_cmd="${2}"
	local ret_code=0
	local msg=

	msg="$(ssh ${ssh_args} "${remote_cmd}" 2>&1)" || ret_code=$?
	if [[ ${ret_code} == 255 ]]; then
		die "${FUNCNAME}(${remote_cmd}): ${msg}"
	elif [[ "${msg}" != "" ]]; then
			echo "${msg}"
	fi

	return ${ret_code}
}

scp_file_upload() {
	local ssh_args="${1}"
	local local_file="${2}"
	local remote_file="${3}"
	local ret_code=0
	local msg=

	msg="$(scp -r ${local_file} ${ssh_args}:${remote_file} 2>&1 > /dev/null)" || ret_code=$?

	if [[ ${ret_code} != 0 ]]; then
		die "${FUNCNAME}(${local_file} ${ssh_args}:${remote_file}: ${msg}"
	fi
}

scp_file_download() {
	local ssh_args="${1}"
	local remote_file="${2}"
	local local_file="${3}"
	local ret_code=0
	local msg=

	msg="$(scp -r ${ssh_args}:${remote_file} ${local_file} 2>&1 >/dev/null)" || ret_code=$?

	if [[ ${ret_code} != 0 ]]; then
		die "${FUNCNAME}(${ssh_args}:${remote_file} ${local_file}): ${msg}"
	fi

}
##### file-name=libgtest.sh, file-mode=755
#!/bin/sh
# GLOBAL VARIABLES USE:
# DIR

set -e

gtest_get_location() {
	if [ -x "/usr/lib/db8/tests/gtest_dbclient_perf" ]; then
		echo "/usr/lib/db8/tests/gtest_dbclient_perf"
	elif [ -x "${DIR}/gtest_dbclient_perf" ]; then
		echo "${DIR}/gtest_dbclient_perf"
	fi
}

gtest_list_tests() {
	local gtest="$(gtest_get_location)"
	local tests="$(${gtest} --gtest_list_tests | grep -v "DISABLED"| grep -e "Sequential" | tr -d '\n' | tr -s ' ')"

	if [[ "${tests}" == "" ]]; then
		die "${FUNCNAME}(${gtest} --gtest_list_tests | grep -v "DISABLED"| grep -e "Sequential" | tr -d '\n' | tr -s ' '): failed"
	else
		echo "${tests}"
	fi
}

gtest_run_test() {
	local dumps_dir="${1}"
	local testname="${2}"
	local outfile="${3}"
	local db_representation="${4}"
	local start=${5}
	local end=${6}
	local step=${7}
	local gtest="$(gtest_get_location)"
	local flags=
	local hdr=
	local body=
	local res=
	local ktime=
	local utime=
	local avg=
	local peak=

	hdr="# dbclient-perf-data-1\n"
	hdr="${hdr}# start=${start}, end=${end}, step=${step}\n"
	hdr="${hdr}# nrecords\t cpu_ktime\t\t cpu_utime\t\t mem_avg\t mem_peak"

	echo -e "${hdr}" >> $outfile
	echo -e "${hdr}"

	for i in $(seq ${start} ${step} ${end})
	do
		if [ "${db_representation}" = "dump" ]; then
			flags="--load-from=${dumps_dir}/dump-$i.txt"
		fi
		res="$(${gtest} ${flags} --gtest_filter=*${testname} --gtest_print_time=0 --nsteps=$i)" || \
			die "test result is not valid"
		ktime=$(echo "$res" | grep -o "ktime = .* sec" | sed 's|ktime = \([0-9]*\.[0-9]*\) sec|\1|') || \
			die "ktime not valid"
		utime=$(echo "$res" | grep -o "utime = .* sec" | sed 's|utime = \([0-9]*\.[0-9]*\) sec|\1|') || \
			die "utime not valid"
		avg=$(echo "$res" | grep -o "avg = .* Kb" | sed 's|avg = \([0-9]*\) Kb|\1|') || \
			die "avg not valid"
		peak=$(echo "$res" | grep -o "peak = .* Kb" | sed 's|peak = \([0-9]*\) Kb|\1|') || \
			die "peak not valid"

		body="${i}\t\t ${ktime}\t\t ${utime}\t\t ${avg}\t\t ${peak}"
		echo -e "${body}" >> $outfile
		echo -e "${body}"
	done
}

gtest_main() {
	local dumps_dir="${1}"
	local db_representation="${2}"
	local start=${3}
	local end=${4}
	local step=${5}
	shift 5
	local all_tests="${@}"

	for testname in ${all_tests}
	do
		if [ -d "${testname}" ]; then
			rm -rf "${testname}"
		fi
		mkdir -p "${testname}" || \
			die "${FUNCNAME}: mkdir(${testname}): failed"
		cd "${testname}" || \
			die "${FUNCNAME}: cd(${testname}): failed"
		inform "Running test ${testname}"
		gtest_run_test "${dumps_dir}" ${testname} "out.txt" "${db_representation}" ${start} ${end} ${step} &
		cd ..
	done
	wait
}

gtest_usage() {
	echo "    --start=NUM               The amount of test database entries to begin with (adjective to 10)"
	echo "                              Default: 10"
	echo "    --end=NUM                 Maximum amount of database entries (adjective to 10)"
	echo "                              Default: 1010"
	echo "    --step=NUM                With each step, the amount of entries increases by NUM"
	echo "                              Default: 1000"
	echo "    --list-tests              List tests to be executed and exit"
	echo "    --db-representation=TYPE  Set how the database will be represented. Can be 'folder' or 'dump'"
	echo "                              Default: dump"
	echo "                              By default, all 'Sequential' tests enabled in gtest_dbclient_perf will be run"
}
##### file-name=main.sh, file-mode=755
#!/bin/sh

set -e
export MOJODB_ENGINE=sandwich
DIR="$(dirname $(readlink -f $0))"
FILE="$(basename $0)"

# GLOBAL VARIABLES THAT MAY BE ALTERED WITH PARSE_ARGS
###################
# DB_REPRESENTATION
# MAKE_PLOTS
# REMOTE_USER
# REMOTE_HOST
# REMOTE_FS
# START
# END
# STEP
###################

# DEFAULT VALUES FOR GLOBAL VARIABLE
###################
# Generate database dumps and use them in tests
# instead of generating database in a loop
DB_REPRESENTATION="dump"
# Make plots after finishing tests unless
# this parameter overridden by command line
# arguments
MAKE_PLOTS="true"
# Default remote user name
REMOTE_USER="root"
# Remote host IP or domain name
REMOTE_HOST=
# Directory on the remote host to upload files
REMOTE_FS="/home/root"
# The amount of test database entries to begin with
START=10
# Maximum amount of database entries
END=1010
# With each step, the amount of entries increases by $STEP
STEP=1000
###################

main() {
	# Database dumps directory path
	local dumps_dir="$(pwd)/dbclient-perf-dumps"
	# Copy gtest application to remote host by default unless
	# it is found there
	local gtest_copy_remote="true"
	# Gtest application path
	local gtest_app=
	# Files to upload to remote host
	local scp_files=
	local all_tests=

	parse_args $@

	if [ "${REMOTE_HOST}" != "" ] && [ -z ${ALREADY_REMOTE} ]; then
		# ssh-copy-id command may hang up in some cases.
		# Due to this reason, the command itself wrapped around with
		# timeout command, so at least will exit and user
		# will have to enter credentials manually later on.
		inform "Copying rsa public key.."
		ssh_copy_id_with_timeout ${REMOTE_USER}@${REMOTE_HOST} 5s

		# Check if remote host already has a gtest application
		# If command below returns one, it means that gtest application
		# already exists on remote host. We do not need to copy it.
		inform "Checking for remote gtest application.."
		ssh_exec_cmd_remote "${REMOTE_USER}@${REMOTE_HOST}" \
			"/bin/sh -c '[ -x /usr/lib/db8/tests/gtest_dbclient_perf ]'" && \
				gtest_copy_remote="false"
	fi

	if [ "${REMOTE_HOST}" != "" ] && [ -z ${ALREADY_REMOTE} ]; then
		# Copy the shell script to the remote machine
		scp_files="${DIR}/${FILE}"

		if [ "${gtest_copy_remote}" = "true" ]; then
			# This usually does not happen, so print a warning
			warning "Gtest application is copied to ${REMOTE_HOST}"
			scp_files="$scp_files ${gtest_app}"
		else
			# Do not copy anything beyond the script file
			:
		fi

		# Upload the script and, maybe, a gtest application to target host
		for file in $scp_files; do
			inform "Uploading $file.."
			scp_file_upload ${REMOTE_USER}@${REMOTE_HOST} ${file} ${REMOTE_FS}
		done

		inform "[remote] Obtaining tests to run.."
		all_tests=$(ssh_exec_cmd_remote "${REMOTE_USER}@${REMOTE_HOST}" "${REMOTE_FS}/${FILE} --list-tests") || \
			die "ssh_exec_cmd_remote("${REMOTE_USER}@${REMOTE_HOST}" "${REMOTE_FS}/${FILE} --list-tests"): failed"
		# Run the tests on remote machine
		inform "[remote] Running tests.."
		ssh_exec_cmd_remote "${REMOTE_USER}@${REMOTE_HOST}" \
			"export ALREADY_REMOTE=true; ${REMOTE_FS}/${FILE} $(printf "%s " ${@} --prefix=${REMOTE_FS} --make-plots=false)" || \
				die "Remote gtest failed with $?"
	else
		gtest_app="$(gtest_get_location)"

		if [ -z "${gtest_app}" ]; then
			die "No local gtest application found"
		fi

		inform "[local] Obtaining tests to run.."
		all_tests="$(gtest_list_tests)"

		if [ "$DB_REPRESENTATION" = "dump" ]; then
			dump_main "${dumps_dir}" ${START} ${END} ${STEP}
		fi
		inform "[local] Running tests.."
		gtest_main "${dumps_dir}" "$DB_REPRESENTATION" ${START} ${END} ${STEP} "${all_tests}"
	fi

	if [ "${REMOTE_HOST}" != "" ] && [ -z ${ALREADY_REMOTE} ]; then
		# Copy test results back to local machine
		for i in ${all_tests}; do
			inform "Downloading ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_FS}/${i}.."
			scp_file_download "${REMOTE_USER}@${REMOTE_HOST}" "${REMOTE_FS}/${i}" "$(pwd)"
		done
	fi

	if [ "${MAKE_PLOTS}" = "true" ]; then
		plot_main "${all_tests}"
	fi

	if [ -z ${ALREADY_REMOTE} ]; then
		inform "All done!"
	fi
}

usage() {
	echo "Usage: $0 [OPTIONS]"
	echo "    --remote-host=HOST        Copy itself to remote host and run. If not specified, runs locally"
	echo "    --remote-user=USER        Remote user's name"
	echo "                              Default: root"
	echo "    --remote-fs=PATH          Remote location"
	echo "                              Default: /home/root"
	echo "    --prefix=PATH             The prefix to override database directory (useful if you want to run test in tmpfs)"
	echo "                              Default: current working directory"
	echo "    --make-plots=true|false   Make plots from test results"
	echo "                              Default: true"
	echo "    --make-dumps              Make test databases represented as json"
	gtest_usage
	echo "    --help                    Print this help and exit"
	exit 0
}

parse_args() {
	for arg in "$@"; do
	case $arg in
		--make-plots=*)
		MAKE_PLOTS="${arg#*=}"
		shift
		;;
		--remote-host=*)
		REMOTE_HOST="${arg#*=}"
		shift
		;;
		--remote-user=*)
		REMOTE_USER="${arg#*=}"
		shift
		;;
		--remote-fs=*)
		REMOTE_FS="${arg#*=}"
		shift
		;;
		--prefix=*)
		GPREFIX="${arg#*=}"
		shift
		;;
		--start=*)
		START="${arg#*=}"
		shift
		;;
		--end=*)
		END="${arg#*=}"
		shift
		;;
		--step=*)
		STEP="${arg#*=}"
		shift
		;;
		--make-dumps)
		dump_main "${DUMPS_DIR}" ${START} ${END} ${STEP}
		exit 0
		;;
		--db-representation=*)
		DB_REPRESENTATION="${arg#*=}"
		shift
		;;
		--help)
		usage
		exit 0
		;;
		--list-tests)
		gtest_list_tests
		exit 0
		;;
		*)
		die "Invalid argument: ${arg}"
		;;
	esac
	done
}

main $@
