function rt = solve_rt(t, k, alpha)
    % Function to solve for rt given t, k, and alpha using numerical methods

    % Define the function to solve
    function y = equation(rt)
        if rt <= 1
            y = Inf;  % Return a large number if rt <= 1 to avoid invalid log values
        else
            term1 = (rt * t - t) / rt;
            term2 = sqrt(2 * ((rt * t -t) * log2(rt) / rt) * (1 - (1 / alpha) * (log2(log2(rt)) / (2 * log2(rt)))));
            y = t-k - (term1 + term2);
        end
    end

    % Initial guess for rt
    rt_initial_guess = 8;  % Initial guess for rt (should be > 1 due to log(rt) and log(log(rt)))

    % Options for fsolve
    options = optimoptions('fsolve', 'Display', 'iter', 'FunctionTolerance', 1e-8, 'StepTolerance', 1e-8);

    % Solve for rt using fsolve
    [rt, fval, exitflag] = fsolve(@equation, rt_initial_guess, options);

    % Check for solution validity
    if exitflag <= 0
        error('The solver did not converge to a solution.');
    end
    if rt <= 1
        error('The computed value of rt is not valid (<= 1).');
    end
end

% Example usage:
t = 1024;  % Given value
k = 25;    % Given value
alpha = 0.99;  % Given value

rt = solve_rt(t, k, alpha);
disp(['The value of rt is: ', num2str(rt)]);

