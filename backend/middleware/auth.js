const jwt = require('jsonwebtoken');
const { pool } = require('../config/database');

function auth(req, res, next) {
  const header = req.headers.authorization;
  if (!header || !header.startsWith('Bearer ')) {
    return res.status(401).json({ error: 'No token provided' });
  }

  const token = header.split(' ')[1];
  try {
    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    req.user = decoded;
    next();
  } catch (err) {
    return res.status(401).json({ error: 'Invalid token' });
  }
}

async function requirePremium(req, res, next) {
  try {
    const result = await pool.query(
      'SELECT plan, premium_until FROM users WHERE id = $1',
      [req.user.id]
    );
    if (result.rows.length === 0) {
      return res.status(401).json({ error: 'User not found' });
    }
    const { plan, premium_until } = result.rows[0];
    const notExpired = !premium_until || new Date(premium_until) > new Date();
    if (plan === 'premium' && notExpired) {
      req.user.plan = 'premium';
      return next();
    }
    return res.status(403).json({
      error: 'Fitur Cloud memerlukan langganan Premium',
      code: 'PREMIUM_REQUIRED'
    });
  } catch (err) {
    console.error('[AUTH] requirePremium error:', err.message);
    return res.status(500).json({ error: 'Server error' });
  }
}

module.exports = auth;
module.exports.requirePremium = requirePremium;
