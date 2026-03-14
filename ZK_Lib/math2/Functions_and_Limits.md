# 函数与极限

[toc]

## 一 映射与函数

---

### 函数的定义

+ 定义:
  + 如果对于每个数$x \in D$,变量y按照一定的法则总有一个确定的y和它对应,则称x为y的函数,记为$y = f(x)$,常称为x为自变量,y为因变量,D为定义域

> [!tip]
> 其实函数就是自变量在其中另一个域的**映射**

+ 定义域: $D_f = D$
+ 值域: $R_f = f(D) = \{y | y = f(x),x\in D\}$

> [!attention]
> 函数概念有两个基本要素:**定义域**,**对应法则**

---

### 函数的特性

1. 函数的有界性
   + 设$X\sub D$
     + 上有界:$\forall x\in X,f(x)\le M_1$
     + 下有界:$\forall x\in X,f(x)\ge M_2$
     + 有界:$\forall x\in X,|f(x)|\le M$
     > [!tip]
     > $\therefore 有界\hArr 有上界且有下界$
     + 无界: $\forall M> 0,\exist x_0 \in X,使|f(x_0)| > M$
     > [例如]
     > $y = \frac{1}{x} 在x < 0 时,上有界,x >0 时 下有界$
2. 函数的单调性
   + 设区间$I \sub D$
     + 单调增: $\forall x_1,x_2 \in I,当x_1 < x_2时,恒有f(x_1) < f(x-2)$
     + 单调减: $\forall x_1,x_2 \in I,当x_1 < x_2时,恒有f(x_1) > f(x-2)$
3. 函数的奇偶性
   + 设D关于原点对称
     + 偶函数:$f(-x) = f(x)~~x\in D$
     + 奇函数:$f(-x) = -f(x)~~x\in D$
     > [!tip]
     > 偶函数图形关于y轴对称,奇函数的图形关于原点对称,且若$f(x)在x = 0 处有定义,则f (0) = 0$
     
4. 函数的周期性
   + 定义
     $$
      若存在实数T > 0 ,对于任意x,恒有f(x + T) = f(x) ,则称为y = f(x)为周期函数,
      使得上式成立的最小正数T称为最小正周期,简称为函数的f(x)的周期 
     $$

---

### 反函数与复合函数
   + 反函数
     + 定义
       $$
        设函数y = f(x)的定义域为D,值域为R_y,若对任意y\in R_y,有唯一确定的x\in D,使得y =f(x),则记为x =f^{-1}(y),称其为y=f(x)的反函数
       $$
     > [!tip]
     > $函数y =f(x)与其反函数y = f^{-1}(x)的图形关于直线y = x对称$

     + 复合函数
       + 定义
         $$
          设y = f(u)的定义域为D_f,u = g(x)的定义域为D_ga,值域为R_g,若D_f\cap R_g \neq \phi,则称函数y = f|g(x)|为函数y = f(u)与u = g(x)的复合函数,它的定义域为\{x|x\in D_g,g(x)\in D_f\}
         $$

---

### 函数的运算
   +  $f\pm g:(f\pm g)(x) = f(x)\pm g(x)$
   +  $f\cdot g:(f\cdot g)(x) = f(x) \cdot g(x)$
   +  $\frac{f}{g}:(\frac{f}{g})(x) = \frac{f(x)}{g(x)}~~~\big(g(x) \neq 0\big)$

---

### 初等函数
   $$
    \begin{cases}
      y = x^{\mu}~~(\mu 为实数)~~幂函数\\
      y = a^x~~(a > 0,a \neq 1)~~指数函数\\
      y = \log_ax~~(aa> 0, a\neq 1)~~对数函数\\
      y = \sin x~~y = \cos x~~ y = \tan x~~y = \cot x~~三角函数\\
      y = \arcsin x~~y = \arccos x~~y = \arctan x~~反三角函数
    \end{cases}
   $$

   + 定义:
     + 由常数和基本初等函数经过有限次的加减乘除和复合所得到且能用一个解析式表达的函数,称为**初等函数**

---

## 二 数列的极限

---

### 数列极限的定义


+ 数列:
  $$x_1,x_2,x_3,x_4,x_5,... 记作\{x_n\}$$


> [例子]
> $1,\frac{1}{2},\frac{1}{3},...,\frac{1}{n}  ~~~x_n = \frac{1}{n}$
> $\frac{1}{2},\frac{2}{3},\frac{3}{4},\frac{5}{6},\frac{7}{8},... ~~~x_n = \frac{n}{n+1}$

+ 数列极限的严格定义:
  $$
    \forall \epsilon > 0 ,\exist N,当n > N时,有|x_n - a| < \epsilon 
  $$
+ 几何意义:
  $$
    \forall \epsilon > 0 ,\exist N ,当n > N时,有x_n \in U(a,\epsilon)
  $$

---

### 收敛数列的性质

1. 唯一性: 收敛数列的极限是唯一的
2. 有界性: 收敛数列必定有界
3. 保号性: $若\lim_{n \rarr \infty}x_n = a, 且a > 0 (a < 0),则\exist N\\ 当n > N时,都有x_n > 0(或x_n < 0)$

> [!important]
> 推论
> $如果存在N > 0,当n > N时,x_n \ge 0 (或x_n \le 0), 则a \ge 0 (或a \le 0)$


4. 收敛数列与其子列之间的关系
   $$
    \lim_{n\rarr \infty}x_n  = a\hArr\lim_{n\rarr \infty}x_{2k -1} = \lim_{n\rarr \infty}x_{2k} = a
   $$

---

## 三 函数的极限

### 函数极限的定义

1. 自变量趋于有限值时函数的极限

+ 定义1:
  $$
    设函数f(x)去点x_0的某个去心领域有定义,若\forall \epsilon > 0 ,\exist \delta > 0 ,当0 < |x - x_0| < \delta时,恒有|f(x) - A| < \epsilon\\
    则称A为x\rarr x_0 时f(x)的极限,记作\lim_{x\rarr x_0}f(x) = A
  $$

+ 左极限定义:
  $$
    \lim_{x\rarr x_{0}^-}f(x) = f(x_{0}^-) = f(x_0 - 0)\\
    若\forall \epsilon > 0 ,\exist \delta > 0 ,当0 < x_0 - x < \delta时,恒有|f(x) - A| < \epsilon\\
  $$

+ 右极限定义:
  $$
    \lim_{x\rarr x_{0}^+}f(x) = f(x_{0}^+) = f(x_0 + 0)\\
    若\forall \epsilon > 0 ,\exist \delta > 0 ,当0 < x - x_0 < \delta时,恒有|f(x) - A| < \epsilon\\
  $$

> [!important]
> 函数左右极限推论
> $$\lim_{x\rarr x_0}f(x) = A \hArr \lim_{x\rarr x_0^-}f(x) = \lim_{x\rarr x_0^+}f(x) = A$$


2.  自变量趋于无穷大时函数的极限

+ 定义二:
  $$
    \begin{split}
      \lim_{x\rarr +\infty}f(x) = A~~~~~~~&\forall \epsilon > 0 ,\exist X > 0 ,当x > X时,恒有|f(x) - A| < \epsilon\\
      \lim_{x\rarr -\infty}f(x) = A~~~~~~~&\forall \epsilon > 0 ,\exist X > 0 ,当x < -X时,恒有|f(x) - A| < \epsilon\\
      \lim_{x\rarr \infty}f(x) = A~~~~~~~&\forall \epsilon > 0 ,\exist X > 0 ,当|x| > X时,恒有|f(x) - A| < \epsilon
    \end{split}
  $$

> [!important]
> 水平渐近线:
> $$若\lim_{x\rarr \infty}f(x) = A,则直线y = A 为y = f(x)的水平渐近线$$
> 函数左右极限(趋近与无限)推论:
> $$\lim_{x\rarr \infty}f(x) = A \hArr \lim_{x\rarr -\infty}f(x) = \lim_{x\rarr +\infty}f(x) = A$$


### 函数极限的性质

1. 唯一性
2. **局部**有界性:
   $$
    \exist M > 0与\delta > 0 ,使得\forall x \in U(x_0,\delta)有|f(x)| \le M
   $$
3. **局部**保号性:
   $$
    如果A > 0 (或A < 0),则存在\delta > 0 ,当x\in U(x_0,\delta)时,f(x) > 0 \big(或f(x) < 0\big)
   $$
   1. 推论1:
      $$
        如果存在\delta >0,当x \in U(x_0,\delta)时,f(x) \ge 0(或f(x) \le 0),那么A\ge 0 (或A \le 0)
      $$
   2. 推论2:
      $$
        如果A \neq 0,则存在\delta >0 ,当x\in U(x_0,\delta)时,|f(x)| > \frac{|A|}{2}
      $$
4. 函数极限与数列极限的关系:
   $$
    若\lim_{x\rarr x_0}f(x) = A,且\lim_{n\rarr \infty}x_n = x_0,x_n \neq x_0,则\lim_{x\rarr \infty}f(x_n) = A
   $$

---


## 四 无穷大与无穷小

### 无穷小

+ 定义1:
  $$
    如果函数f(x)当x\to x_0(或x\to \infty)时的极限为零,则称f(x)为x\to x_0(或x\to \infty)时的无穷小量
  $$
  > [例子]
  > $$\begin{split}\lim_{x\to 0}\sin x = 0~~~~~~~~~~~~~&\lim_{x\to \infty}\frac{1}{x}= 0\\\lim_{x\to 0^-}e^{\frac{1}{x}}= 0~~~~~~~~~~~~~&lim_{n\to \infty}\frac{1}{n}= 0\end{split}$$

  > [!attention]
  > 1. 无穷小是变量,不能与很小的数混淆
  > 2. 零是可以作为无穷小的唯一的数


+ 无穷小定理一:
  $$
    \lim f(x) = A \hArr f(x) = \alpha (x) + A ,其中\lim \alpha(x) = 0
  $$


### 无穷大
+ 定义二:
  $$
    \lim_{x\to x_0}f(x) = \infty,则称f(x)为x\to x_0(或x\to \infty)时的无穷大量\\
    即:若对任意给定的M>0,总存在\delta > 0 ,当0<|x - x_0| < \delta时,恒有|f(x)| > M
  $$
  + 正无穷大:
    $$
      \lim_{x\to x_0}f(x) = +\infty
    $$
  + 负无穷大:
    $$
      \lim_{x\to x_0}f(x) = -\infty
    $$
+ 无穷大的几何意义
  1. $若\lim_{x\to x_0}f(x) = \infty则x = x_0为y = f(x)的垂直渐近线$
  2. $  若\lim_{x\to \infty}f(x) = a则y = a为y = f(x)的水平渐近线$

+ 无穷大定理二:
  $$
    在同一极限过程中,如果f(x)是无穷大,则\frac{1}{f(x)}时无穷小,反之,如果f(x)时无穷小,且f(x) \neq 0,则\frac{1}{f(x)}是无穷大
  $$

---

## 五 极限运算法则

+ 定理1: 
  $$
    两个无穷小的和是无穷小
  $$
+ 定理2:
  $$
    有界函数与无穷小的乘积是无穷小
  $$
+ 推论:**有限个**无穷小的和为无穷小


+ 推论1:$$常数与无穷小的乘积是无穷小$$
  + 推论2:$$有限个无穷小的积仍是无穷小$$
+ 定理3: $$若\lim f(x) = A,\lim g(x) = B,那么\\\lim\big(f(x)\pm g(x)\big) = \lim f(x) \pm \lim g(x)\\\lim\big(f(x)\cdot g(x)\big) = \lim f(x) \cdot \lim g(x)\\\lim \big(\frac{f(x)}{g(x)}\big) = \frac{\lim f(x)}{\lim g(x)}~~~(B\neq 0)$$

> ![important]
> $$极限存在 \pm 极限不存在 = 极限不存在$$a
> $$不存在 \pm 不存在 = 不一定$$
> $$\frac{存在}{不存在} = 不一定$$
> $$不存在 \cdot 不存在 = 不一定$$


+ 推论1: $$如果\lim f(x)存在,而c为常数,那么\lim [cf(x)] = c\lim f(x)$$
+ 推论2: $$如果\lim f(x)存在,而n为正整数,那么\lim [f(x)]^n = [\lim f(x)]^n$$

+ 定理4: 
$$\begin{split}&~~~~~~设\lim_{n \to \infty}a_n = a,\lim_{n \to \infty}b_n = b,则\\ 
&(1)\lim_{n\to \infty}(a_n\pm b_n) = \lim_{n\to \infty}a_n \pm \lim_{n \to \infty}b_n = a \pm b\\
&(2)\lim_{n\to \infty}(a_n \cdot b_n) = \lim_{n\to \infty}a_n \cdot \lim_{n \to \infty}b_n = a\cdot b\\
&(3)\lim_{n\to \infty}\frac{a_n}{b_n} =\frac{\lim_{n\to \infty}a_n }{\lim_{n \to \infty}b_n} = \frac{a}{b}~~~(b\neq 0)\end{split}\\
$$

> [!important]
> 常见的结论
> 1. $\lim f(x) = A \neq 0 \rArr \lim f(x)g(x) = A\lim g(x)$
> 2. $\lim \frac{f(x)}{g(x)}存在,\lim g(x) = 0 \rArr \lim f(x) = 0$
> 3. $\lim \frac{f(x)}{g(x)} = A \neq 0,\lim f(x) = 0\rArr \lim g(x) = 0$

+ 抓大头:
  $$
    \lim_{x\to \infty}\frac{a_nx^n + a_{n - 1}^{n-1}+ ... + a_1x + a_0}{b_nx^n + b_{n - 1}^{n-1}+ ... + b_1x + b_0} = \begin{cases}
      \frac{a_n}{b_n},~~~n = m\\
      0,~~~n < m\\
      \infty,~~~n>m
    \end{cases}
  $$

+ 定理5: $$如果\varphi(x) \ge \psi(x),而\lim\varphi(x) = A,\lim\psi(x) = B,那么A\ge B$$

+ 定理6: $$设y = f[g(x)]是由y = f(u),u = g(x)复合而成,\lim_{x\to x_0}g(x)= u_0且\lim_{u\to u_0}f(u) = a,当x\in U(x_0,\delta_0)时,g(x) \neq u_0,则\lim_{x\to x_0}f[g(x)] = a$$


---

## 六 极限存在法则与两个重要极限

### 夹逼准则

+ 准则1: 
$$
\begin{split}
  &~~~如果数列\{x_n\},\{x_n\},及\{x_n\}满足下列条件:\\
  &1. 存在N,当n < N时,x_n\le y_n \le z_n\\
  &2. \lim_{n\to \infty}x_n = \lim_{n\to \infty}z_n = a\\
  &则\lim_{n\to \infty}y_n =  a
\end{split}
$$

+ 准则1`:
$$
\begin{split}
  &1. 当x\in U(x_0,\delta)时,f(x)\le g(x) \le h(x) \\
  &2. 当\lim_{x\to x_0}f(x) = \lim_{x\to x_0}h(x) = a\\
  & 则\lim_{x\to x_0}g(x) = a
\end{split}
$$


+ 重要极限:
  1. $\lim_{x\to x_0}\frac{\sin x}{x} = \lim_{x\to \Delta}\frac{\sin \Delta}{\Delta} = 1$
   
### 单调有界准则

+ 准则2: 单调有界数列必有极限,即单调增(减)有上(下)界的数列必有极限

+ 重要极限2:
  + $\lim_{x\to \infty}(1 + \frac{1}{x})^{x} = \lim_{x\to 0}(1 + x)^{\frac{1}{x}} = \lim_{x\to 0}(1 + \Delta)^{\frac{1}{\Delta}} = \lim_{x\to \infty}(1 + \frac{1}{\Delta})^{\Delta} = e$

> [!important]
> 常用基本极限
> 1. $\lim_{x\to 0}\frac{\sin x}{x} = 1$
> 2. $\lim_{x\to 0}(1 + x)^{\frac{1}{x}} = \lim_{x\to \infty}(1 + \frac{1}{x})^{x} = e$
> 3. $\lim_{x\to 0}\frac{a^x - 1}{x} = \ln a$
> 4. $\lim_{n\to \infty}\sqrt[n]{n} = 1$
> 5. $\lim_{n\to \infty}\sqrt[n]{a} = 1~(a > 1)$
> 6. $\lim_{x\to \infty}\frac{a_nx^n + a_{n - 1}^{n-1}+ ... + a_1x + a_0}{b_nx^n + b_{n - 1}^{n-1}+ ... + b_1x + b_0} = \begin{cases}\frac{a_n}{b_n},~~~n = m\\0,~~~n < m\\\infty,~~~n>m\end{cases}$
> 7. $\lim_{x\to \infty}x^n=\begin{cases}0,|x| < 1\\\infty,|x| > 1\\1,x = 1\\不存在,x = -1\end{cases}$
> 8. $\lim_{x\to \infty}e^{nx}=\begin{cases}0,x < 0\\+\infty,x > 0\\1,x = 0\end{cases}$

> [!important]
> $1^{\infty}型极限常用结论$
> $若\lim\alpha(x) = 0,\lim\beta(x) = \infty,且\lim\alpha(x)\beta(x) = A,则$
> $$\lim[1 + \alpha(x)]^{\beta(x)} = e^A$$

> [!important]
> 泰勒公式:
> $设f(x)在x == x_0 处n阶可导,则$
> $$f(x) = f(x_0) + f'(x_0)(x - x_0)+ ... + \frac{f^{n}(x_0)}{n!} + o(x - x_0)^n$$
> 常见泰勒公式
> $e^x = 1 + x + \frac{x^2}{2!} + ... + \frac{x^n}{n!} + o(x^n)$
> $\sin x = x - \frac{x^3}{3!} + ... + (-1)^{n-1}\frac{x^{2n-1}}{(2n-1)!} + o(x^{2n-1})$
> $\cos x = 1 - \frac{x^2}{2!} + ... + (-1)^n\frac{x^{2n}}{(2n)!} + o(x^{2n})$
> $\ln (1 + x) = x - \frac{x^2}{2} + ... + (-1)^{n-1}\frac{x^n}{n} + o(x^n)$
> $(1 + n)^\alpha = 1 + \alpha + \frac{\alpha(\alpha - 1)}{2!} + ... + \frac{\alpha(\alpha - 1)...(\alpha - n + 1)}{n!} + o(x^n)$

## 七 无穷小的比较

+ 定义:
  1. $若\lim\frac{\alpha(x)}{\beta(x)} = 0,则称为\alpha(x)是\beta(x)的高阶无穷小,记作\alpha(x) = o(\beta(x))$
  2. $若\lim\frac{\alpha(x)}{\beta(x)} = \infty,则称为\alpha(x)是\beta(x)的低阶无穷小$
  3. $若\lim\frac{\alpha(x)}{\beta(x)} = a \neq 0,则称为\alpha(x)是\beta(x)的同阶无穷小$
  4. $若\lim\frac{\alpha(x)}{\beta(x)} = 1,则称为\alpha(x)是\beta(x)的等阶无穷小,记作\alpha(x)\sim\beta(x)$
  4. $若\lim\frac{\alpha(x)}{[\beta(x)]^k} = a \neq 0,k>0,则称为\alpha(x)是\beta(x)的k阶无穷小$
  
+ 定理1:
  $$
    \alpha(x)\sim \beta(x)的充要条件是\alpha(x) = \beta(x) + o(\beta(x))
  $$ 

+ 定理2:
  $$
    设\alpha(x)\sim \alpha_1(x),\beta(x)\sim \beta_1(x),且\lim\frac{\alpha_1(x)}{\beta_1(x)},则\\
    \lim\frac{\alpha(x)}{\beta(x)} = \lim\frac{\alpha_1(x)}{\beta_1(x)}
  $$ 

> [!important]
> 常见的等价无穷小
> 1. $x \sim \sin x\sim \tan x\sim \arcsin x\sim \arctan x\sim \ln (1 + x)\sim e^x - 1$
> 2. $1 - \cos x \sim \frac{1}{2}x^2$
> 3. $(1 + x)^{a} -  1\sim ax$
> 4. $a^x - 1\sim x\ln a$
> 5. $x - \sin x  \sim \frac{1}{6}x^3$
> 6. $\tan x - x \sim \frac{1}{3}x^3$
> 7. $x - \ln (1 + x) \sim \frac{1}{2}x^2$
> 8. $\arcsin x - x\sim \frac{1}{6}x^3$
> 9. $x - \arctan x \sim \frac{1}{3}x^3$

> [!important]
> 加减关系在一定条件下可以换
> 1. $\alpha \sim \alpha_1,\beta \sim \beta_{1},且\lim\frac{\alpha_1}{\beta_1} = A \neq 1,则\alpha - \beta \sim \alpha_1 - \beta_1$
> 2. $\alpha \sim \alpha_1,\beta \sim \beta_{1},且\lim\frac{\alpha_1}{\beta_1} = A \neq -1,则\alpha + \beta \sim \alpha_1 + \beta_1$

## 八 函数连续性与间断点

### 函数的连续性

+ 定义(连续)
$$
  若\lim_{\Delta x \to 0}\Delta y = 0,或\lim_{x\to x_0}f(x) = f(x_0),则称为f(x)在x_0处连续
$$
+ 左连续
$$
  \lim_{x\to x_0^-}f(x) =f(x_0)
$$
+ 右连续
$$
  \lim_{x\to x_0^+}f(x) =f(x_0)
$$

+ 推论:
  $$
    连续\hArr 左连续且右连续
  $$

### 函数的间断点

+ 连续
$f(x)在x-0连续$ 
1. $f(x)在x_0有定义$
2. $\lim_{x\to x_0}f(x)存在$
3. $\lim_{x\to x_0}f(x) = f(x_0)$

+ 间断点
$f(x)在x_0某去心领域有定义$
1. $f(x)在x_0无定义$
2. $\lim_{x\to x_0}f(x)不存在$
3. $\lim_{x\to x_0}f(x) \neq f(x_0)$

+ 间断点分类:
  1. 第一类间断点:(左右极限都存在)
     1. 可去间断点($f(x - x_0) = f(x + x_0)$)
     2. 跳跃间断点($f(x - x_0) \neq f(x + x_0)$)
  2. 第二类间断点:(左右极限q其中之一不存在)


## 九 连续函数的运算与初等函数的连续性

### 连续函数的和差积商的连续性
1. 定理一:
   $$
    设函数
   $$
### 反函数与复合函数的连续性
2. 定理二:
   $$
    设f:[a,b]\to R是严格单调增(减)的连续函数,则其反函数在[f(a),f(b)],([f(b),f(a)]),上也是连续的
   $$