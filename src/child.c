/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   child.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: daflynn <daflynn@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/13 19:56:15 by daflynn           #+#    #+#             */
/*   Updated: 2025/09/13 19:56:21 by daflynn          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

void	ft_appropriate_child_signal(char *str)
{
	if (ft_is_builtin(str) && ft_is_pipe_builtin(str))
		ft_set_main_signals();
	else
		ft_set_child_signals();
}

void	ft_child_process(t_dat *d, char ***cmd, int **fd, size_t i)
{
	t_rdr	r;
	int		len;

	if (!cmd || !cmd[i] || !cmd[i][0])
		exit(1);
	len = 0;
	while (cmd[i][len])
		len++;
	ft_memset(&r, 0, sizeof(r));
	if (!ft_validate_segment(cmd[i], 0, len))
		exit(1);
	ft_setup_io(fd, i, d->tot);
	ft_close_pipes(fd, d->tot);
	ft_appropriate_child_signal(cmd[i][0]);
	ft_parse_redirection(cmd[i], &r);
	if (!ft_apply_redirections(&r, cmd[i]))
	{
		ft_free_redirection(&r);
		exit(1);
	}
	ft_exec_command(d, cmd[i]);
}

void	ft_fork_children(t_dat *d, char ***cmd, int **fd)
{
	pid_t	pid;
	size_t	i;

	i = 0;
	while (i < d->tot)
	{
		if (!cmd[i] || !cmd[i][0])
		{
			d->pids[i] = -1;
			i++;
			continue ;
		}
		pid = fork();
		if (pid == 0)
		{
			ft_set_child_signals();
			ft_child_process(d, cmd, fd, i);
		}
		else if (pid < 0)
			perror("fork");
		d->pids[i] = pid;
		i++;
	}
}

void	ft_nested_child(t_dat *d, char **cmd, char *cmd_path, int s_stdin)
{
	close(s_stdin);
	cmd_path = ft_get_cmd_path(d, cmd[0], 0);
	if (!cmd_path)
	{
		ft_cmd_not_found(cmd[0]);
		exit(1);
	}
	execve(cmd_path, cmd, d->evs);
	exit(1);
}

void	ft_wait_children(t_dat *d)
{
	int status;
	size_t i;
	int signal_num;
	int last_exit_code;
	pid_t last_pid;

	last_exit_code = 0;
	last_pid = d->pids[d->tot - 1];
	i = 0;
	while (i < d->tot)
	{
		if (d->pids[i] == -1)
		{
			i++;
			continue ;
		}
		signal(SIGINT, SIG_IGN);
		waitpid(d->pids[i], &status, 0);
		if (d->pids[i] == last_pid)
		{
			if (WIFSIGNALED(status))
			{
				signal_num = WTERMSIG(status);
				if (signal_num == SIGQUIT)
					(printf("quit: core dumped\n"), last_exit_code = 131);
				else if (signal_num == SIGINT)
					(write(1, "\n", 1), last_exit_code = 130);
			}
			else if (WIFEXITED(status))
				last_exit_code = WEXITSTATUS(status);
		}
		i++;
	}
	ft_set_main_signals();
	g_last_exit_status = last_exit_code;
}